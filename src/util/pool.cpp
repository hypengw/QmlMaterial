#include "qml_material/util/pool.hpp"

#include <QtCore/QMetaObject>
#include <QtQml/QQmlContext>
#include <algorithm>
#include <atomic>
#include <utility>
#include <vector>

#include "qml_material/util/loggingcategory.hpp"
#include "qml_material/util/qml_util.hpp"

namespace
{
static std::atomic<std::size_t> the_pool_obj_count { 0 };

void object_destroyed() { the_pool_obj_count--; }

auto variant_map_to_string(const QVariantMap& map) -> QString {
    QString out;
    for (const auto& el : map.asKeyValueRange()) {
        out.append(QString("%1%2").arg(el.first).arg(el.second.toString()));
    }
    return out;
}

auto errors_to_string(const QList<QQmlError>& errors) -> QString {
    QStringList lines;
    lines.reserve(errors.size());
    for (const auto& error : errors) {
        lines.push_back(error.toString());
    }
    return lines.join(u'\n');
}
} // namespace

namespace qml_material
{

auto pool_object_count() -> std::size_t { return the_pool_obj_count; }

PoolRequest::PoolRequest(Pool* pool, qint64 id, QVariant key)
    : QObject(pool), m_pool(pool), m_id(id), m_key(std::move(key)) {}

auto PoolRequest::id() const -> qint64 { return m_id; }
auto PoolRequest::key() const -> QVariant { return m_key; }
auto PoolRequest::status() const -> Status { return m_status; }
auto PoolRequest::progress() const -> qreal { return m_progress; }
auto PoolRequest::object() const -> QObject* { return m_object; }
auto PoolRequest::cached() const -> bool { return m_cached; }
auto PoolRequest::errorString() const -> QString { return m_errorString; }

void PoolRequest::cancel() {
    if (m_pool) m_pool->cancelRequest(this);
}

void PoolRequest::forceCompletion() {
    if (m_pool) m_pool->forceRequest(this);
}

void PoolRequest::release() {
    if (m_pool) m_pool->releaseRequest(this);
}

void PoolRequest::setStatus(Status value) {
    if (m_status == value) return;
    m_status = value;
    emit statusChanged();
}

void PoolRequest::setProgress(qreal value) {
    value = std::clamp(value, qreal(0), qreal(1));
    if (qFuzzyCompare(m_progress, value)) return;
    m_progress = value;
    emit progressChanged();
}

void PoolRequest::setObject(QObject* value) {
    if (m_object == value) return;
    m_object = value;
    emit objectChanged();
}

void PoolRequest::setCached(bool value) {
    if (m_cached == value) return;
    m_cached = value;
    emit cachedChanged();
}

void PoolRequest::setErrorString(QString value) {
    if (m_errorString == value) return;
    m_errorString = std::move(value);
    emit errorStringChanged();
}

PoolIncubator::PoolIncubator(Pool* pool, qint64 id, IncubationMode mode)
    : QQmlIncubator(mode), m_pool(pool), m_id(id) {}

void PoolIncubator::statusChanged(Status status) {
    QQmlIncubator::statusChanged(status);
    m_pool->incubatorStateChanged(m_id, status);
}

void PoolIncubator::setInitialState(QObject* object) {
    QQmlIncubator::setInitialState(object);
    m_pool->setInitialState(m_id, object);
}

Pool::Pool(QObject* parent): QObject(parent) {}

Pool::~Pool() {
    m_destroying = true;

    for (auto& [id, task] : m_tasks) {
        Q_UNUSED(id);
        task->terminal = true;
        if (task->incubator) task->incubator->clear();
    }
    m_tasks.clear();

    for (auto& [key, entry] : m_cache) {
        Q_UNUSED(key);
        if (entry.object) delete entry.object;
    }
    m_cache.clear();

    for (auto& [object, request] : m_uncached) {
        Q_UNUSED(request);
        delete object;
    }
    m_uncached.clear();
}

auto Pool::request(const QVariant& source, const QVariantMap& initialProperties,
                   const QVariant& key, IncubationMode mode) -> PoolRequest* {
    const bool    hasKey    = key.isValid() && ! key.isNull();
    const QString keyString = hasKey ? key.toString() : QString {};

    if (hasKey) {
        if (auto it = m_inflightKeys.find(keyString); it != m_inflightKeys.end()) {
            if (it->second) return it->second;
            m_inflightKeys.erase(it);
        }

        if (auto it = m_cache.find(keyString); it != m_cache.end()) {
            if (! it->second.object) {
                m_cache.erase(it);
            } else {
                auto* result = new PoolRequest(this, genSerial(), key);
                if (it->second.lease) {
                    result->setErrorString(
                        QString("pool key is already in use: %1").arg(keyString));
                    result->setStatus(PoolRequest::Error);
                    return result;
                }

                it->second.lease = result;
                result->setObject(it->second.object);
                result->setCached(true);
                result->setProgress(1);
                result->setStatus(PoolRequest::Ready);
                return result;
            }
        }
    }

    const auto id     = genSerial();
    auto*      result = new PoolRequest(this, id, key);
    result->setStatus(PoolRequest::Loading);

    auto  task    = std::make_unique<Task>(Task {
        .id                = id,
        .source            = source,
        .initialProperties = initialProperties,
        .key               = keyString,
        .hasKey            = hasKey,
        .mode              = mode,
        .request           = result,
    });
    auto* taskPtr = task.get();
    m_tasks.emplace(id, std::move(task));
    if (hasKey) m_inflightKeys.insert_or_assign(keyString, result);
    connect(result, &QObject::destroyed, this, [this, id]() {
        if (auto it = m_tasks.find(id); it != m_tasks.end() && ! it->second->terminal) {
            it->second->request = nullptr;
            finishCancelled(id);
        }
    });

    const auto compilationMode =
        mode == Asynchronous ? QQmlComponent::Asynchronous : QQmlComponent::PreferSynchronous;
    if (! createComponent(*taskPtr, compilationMode)) {
        finishError(id, taskPtr->sourceError);
        return result;
    }

    auto* component = taskPtr->component.data();
    connect(component, &QQmlComponent::statusChanged, this, [this, id]() {
        onComponentStatusChanged(id);
    });
    connect(component, &QQmlComponent::progressChanged, this, [this, id](qreal progress) {
        onComponentProgressChanged(id, progress);
    });
    connect(component, &QObject::destroyed, this, [this, id]() {
        if (auto it = m_tasks.find(id); it != m_tasks.end() && ! it->second->terminal) {
            finishError(id, QStringLiteral("component was destroyed while loading"));
        }
    });

    onComponentProgressChanged(id, component->progress());
    onComponentStatusChanged(id);
    return result;
}

auto Pool::createComponent(Task& task, QQmlComponent::CompilationMode mode) -> bool {
    if (auto* component = task.source.value<QQmlComponent*>()) {
        task.component = component;
        return true;
    }

    auto resolved    = resolveComponentSource(task.source, qmlEngine(this), qmlContext(this), mode);
    task.sourceError = std::move(resolved.errorString);
    task.component   = resolved.component;
    task.ownedComponent = std::move(resolved.ownedComponent);
    return task.component != nullptr;
}

void Pool::onComponentStatusChanged(qint64 id) {
    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& task = *it->second;
    if (task.terminal || ! task.component) return;

    switch (task.component->status()) {
    case QQmlComponent::Ready: startIncubation(id); break;
    case QQmlComponent::Error: finishError(id, taskError(task)); break;
    case QQmlComponent::Null:
    case QQmlComponent::Loading: break;
    }
}

void Pool::onComponentProgressChanged(qint64 id, qreal progress) {
    if (auto it = m_tasks.find(id); it != m_tasks.end()) {
        if (it->second->request) it->second->request->setProgress(progress);
    }
}

void Pool::startIncubation(qint64 id) {
    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& task = *it->second;
    if (task.terminal || task.incubator || ! task.component) return;

    task.incubator = std::make_unique<PoolIncubator>(this, id, toQtMode(task.mode));
    task.incubator->setInitialProperties(task.initialProperties);
    auto* context = task.component->creationContext();
    if (! context) context = qmlContext(this);
    if (! context && task.component->engine()) context = task.component->engine()->rootContext();
    task.component->create(*task.incubator, context, qmlContext(this));
}

void Pool::incubatorStateChanged(qint64 id, QQmlIncubator::Status status) {
    if (m_destroying || status == QQmlIncubator::Loading || status == QQmlIncubator::Null) return;

    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& task = *it->second;
    if (task.terminal || ! task.incubator) return;

    if (status == QQmlIncubator::Ready) {
        auto* object = task.incubator->object();
        task.incubator->clear();
        finishReady(id, object);
        return;
    }

    auto error = errors_to_string(task.incubator->errors());
    if (auto* object = task.incubator->object()) delete object;
    task.incubator->clear();
    finishError(id, error.isEmpty() ? QStringLiteral("object incubation failed") : error);
}

void Pool::setInitialState(qint64, QObject*) {}

void Pool::finishReady(qint64 id, QObject* object) {
    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& task = *it->second;
    if (task.terminal) return;
    if (! object) {
        finishError(id, QStringLiteral("object incubation completed without an object"));
        return;
    }

    task.terminal = true;
    removeInflightKey(task);

    auto* result = task.request.data();
    if (! result) {
        scheduleTaskCleanup(id);
        object->deleteLater();
        return;
    }

    the_pool_obj_count++;
    connect(object, &QObject::destroyed, object_destroyed);

    if (task.hasKey) {
        const auto generation = id;
        m_cache.insert_or_assign(task.key,
                                 CacheEntry {
                                     .object     = object,
                                     .lease      = result,
                                     .generation = generation,
                                 });
        connect(object, &QObject::destroyed, this, [this, key = task.key, generation]() {
            onCacheObjectDestroyed(key, generation);
        });
    } else {
        m_uncached.insert_or_assign(object, result);
        connect(object, &QObject::destroyed, this, [this, object]() {
            onUncachedObjectDestroyed(object);
        });
    }

    scheduleTaskCleanup(id);
    if (! result) return;
    result->setObject(object);
    result->setCached(task.hasKey);
    result->setProgress(1);
    result->setStatus(PoolRequest::Ready);
}

void Pool::finishError(qint64 id, QString error) {
    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& task = *it->second;
    if (task.terminal) return;

    task.terminal = true;
    removeInflightKey(task);
    scheduleTaskCleanup(id);

    if (error.isEmpty()) error = QStringLiteral("dynamic object creation failed");
    qCWarning(qml_material_logcat()) << error;
    if (auto* result = task.request.data()) {
        result->setErrorString(std::move(error));
        result->setStatus(PoolRequest::Error);
    }
}

void Pool::finishCancelled(qint64 id) {
    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& task = *it->second;
    if (task.terminal) return;

    task.terminal = true;
    removeInflightKey(task);
    if (task.incubator) task.incubator->clear();
    scheduleTaskCleanup(id);

    if (auto* result = task.request.data()) {
        result->deleteLater();
        result->setStatus(PoolRequest::Cancelled);
    }
}

void Pool::scheduleTaskCleanup(qint64 id) {
    QMetaObject::invokeMethod(
        this,
        [this, id]() {
            m_tasks.erase(id);
        },
        Qt::QueuedConnection);
}

void Pool::removeInflightKey(const Task& task) {
    if (! task.hasKey) return;
    auto it = m_inflightKeys.find(task.key);
    if (it != m_inflightKeys.end() && it->second == task.request) m_inflightKeys.erase(it);
}

auto Pool::taskError(const Task& task) const -> QString {
    if (! task.component) return QStringLiteral("component is no longer available");
    const auto errors = errors_to_string(task.component->errors());
    return errors.isEmpty() ? task.component->errorString() : errors;
}

void Pool::forceRequest(PoolRequest* result) {
    if (! result || result->status() != PoolRequest::Loading) return;
    auto it = m_tasks.find(result->id());
    if (it == m_tasks.end()) return;
    auto& task = *it->second;

    if (task.component && task.component->isLoading()) {
        if (! task.ownedComponent) return;

        QObject::disconnect(task.component, nullptr, this, nullptr);
        task.component = nullptr;
        task.ownedComponent.reset();
        if (! createComponent(task, QQmlComponent::PreferSynchronous)) {
            finishError(task.id,
                        task.sourceError.isEmpty()
                            ? QStringLiteral("cannot synchronously reload component")
                            : task.sourceError);
            return;
        }

        auto* component = task.component.data();
        connect(component, &QQmlComponent::statusChanged, this, [this, id = task.id]() {
            onComponentStatusChanged(id);
        });
        connect(
            component, &QQmlComponent::progressChanged, this, [this, id = task.id](qreal progress) {
                onComponentProgressChanged(id, progress);
            });
        connect(component, &QObject::destroyed, this, [this, id = task.id]() {
            if (auto current = m_tasks.find(id);
                current != m_tasks.end() && ! current->second->terminal) {
                finishError(id, QStringLiteral("component was destroyed while loading"));
            }
        });
        const auto id = task.id;
        onComponentStatusChanged(id);
        if (auto current = m_tasks.find(id);
            current != m_tasks.end() && ! current->second->terminal && current->second->incubator &&
            current->second->incubator->isLoading()) {
            current->second->incubator->forceCompletion();
        }
        return;
    }

    if (task.incubator && task.incubator->isLoading()) task.incubator->forceCompletion();
}

void Pool::cancelRequest(PoolRequest* result) {
    if (! result) return;
    if (result->status() == PoolRequest::Ready) {
        releaseRequest(result);
        return;
    }
    if (result->status() == PoolRequest::Loading) {
        finishCancelled(result->id());
        return;
    }
    if (result->status() == PoolRequest::Error || result->status() == PoolRequest::Cancelled) {
        result->deleteLater();
    }
}

void Pool::releaseRequest(PoolRequest* result) {
    if (! result) return;
    if (result->status() == PoolRequest::Loading) {
        finishCancelled(result->id());
        return;
    }
    if (result->status() == PoolRequest::Released) return;

    auto* object = result->object();
    if (result->cached()) {
        const auto key = result->key().toString();
        if (auto it = m_cache.find(key); it != m_cache.end() && it->second.object == object) {
            if (it->second.lease == result) it->second.lease = nullptr;
            if (it->second.evictOnRelease) {
                auto tracked = it->second.object;
                m_cache.erase(it);
                if (tracked) tracked->deleteLater();
            }
        }
    } else if (object) {
        m_uncached.erase(object);
        object->deleteLater();
    }

    result->deleteLater();
    result->setObject(nullptr);
    result->setStatus(PoolRequest::Released);
}

void Pool::onCacheObjectDestroyed(const QString& key, qint64 generation) {
    if (m_destroying) return;
    auto it = m_cache.find(key);
    if (it == m_cache.end() || it->second.generation != generation) return;

    auto request = it->second.lease;
    m_cache.erase(it);
    if (request && request->status() == PoolRequest::Ready) {
        request->setObject(nullptr);
        request->setErrorString(QString("cached object was destroyed: %1").arg(key));
        request->setStatus(PoolRequest::Error);
    }
}

void Pool::onUncachedObjectDestroyed(QObject* object) {
    if (m_destroying) return;
    auto it = m_uncached.find(object);
    if (it == m_uncached.end()) return;

    auto request = it->second;
    m_uncached.erase(it);
    if (request && request->status() == PoolRequest::Ready) {
        request->setObject(nullptr);
        request->setErrorString(QStringLiteral("dynamic object was destroyed externally"));
        request->setStatus(PoolRequest::Error);
    }
}

auto Pool::contains(const QString& key) const -> bool {
    if (auto it = m_cache.find(key); it != m_cache.end()) return bool(it->second.object);
    return false;
}

auto Pool::get(const QString& key) const -> QObject* {
    if (auto it = m_cache.find(key); it != m_cache.end()) return it->second.object;
    return nullptr;
}

auto Pool::evict(const QVariant& key) -> bool {
    if (! key.isValid() || key.isNull()) return false;
    const auto keyString = key.toString();
    bool       found     = false;

    if (auto it = m_inflightKeys.find(keyString); it != m_inflightKeys.end()) {
        auto request = it->second;
        found        = true;
        if (request) cancelRequest(request);
    }

    auto it = m_cache.find(keyString);
    if (it == m_cache.end()) return found;
    found = true;
    if (it->second.lease) {
        it->second.evictOnRelease = true;
    } else {
        auto object = it->second.object;
        m_cache.erase(it);
        if (object) object->deleteLater();
    }
    return found;
}

void Pool::clear() {
    std::vector<QPointer<PoolRequest>> pending;
    pending.reserve(m_tasks.size());
    for (const auto& [id, task] : m_tasks) {
        Q_UNUSED(id);
        pending.push_back(task->request);
    }
    for (auto request : pending) {
        if (request) cancelRequest(request);
    }

    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (it->second.lease) {
            it->second.evictOnRelease = true;
            ++it;
        } else {
            auto object = it->second.object;
            it          = m_cache.erase(it);
            if (object) object->deleteLater();
        }
    }
}

void Pool::add(const QVariant& source, const QVariantMap& initialProperties, bool autoKey) {
    const auto mode = async() ? Asynchronous : Synchronous;
    if (autoKey) {
        const auto key =
            QString("%1%2").arg(source.toString()).arg(variant_map_to_string(initialProperties));
        attachLegacy(request(source, initialProperties, key, mode));
    } else {
        attachLegacy(request(source, initialProperties, {}, mode));
    }
}

void Pool::addWithKey(const QString& key, const QVariant& source,
                      const QVariantMap& initialProperties) {
    attachLegacy(request(source, initialProperties, key, async() ? Asynchronous : Synchronous));
}

void Pool::add(std::optional<QStringView> key, QQmlComponent* component,
               const QVariantMap& initialProperties) {
    const auto keyValue = key ? QVariant(key->toString()) : QVariant {};
    attachLegacy(request(QVariant::fromValue(component),
                         initialProperties,
                         keyValue,
                         async() ? Asynchronous : Synchronous));
}

void Pool::attachLegacy(PoolRequest* request) {
    if (! request) return;
    m_legacyQueue.push_back(request);
    connect(request, &PoolRequest::statusChanged, this, &Pool::queueLegacyDrain);
    queueLegacyDrain();
}

void Pool::queueLegacyDrain() {
    if (m_legacyDrainPending) return;
    m_legacyDrainPending = true;
    QMetaObject::invokeMethod(
        this,
        [this]() {
            m_legacyDrainPending = false;
            drainLegacy();
        },
        Qt::QueuedConnection);
}

void Pool::drainLegacy() {
    while (! m_legacyQueue.empty()) {
        auto request = m_legacyQueue.front();
        if (! request) {
            m_legacyQueue.pop_front();
            continue;
        }
        if (request->status() == PoolRequest::Null || request->status() == PoolRequest::Loading) {
            return;
        }

        m_legacyQueue.pop_front();
        if (request->status() == PoolRequest::Ready) {
            auto* object = request->object();
            auto  key    = request->key();
            if (request->cached()) releaseRequest(request);
            emit objectAdded(object, key);
        } else {
            releaseRequest(request);
        }
    }
}

auto Pool::async() const -> bool { return m_async; }

void Pool::setAsync(bool value) {
    if (m_async == value) return;
    m_async = value;
    emit asyncChanged(value);
}

auto Pool::removeObject(QObject* object) -> bool {
    if (! object) return false;
    if (auto it = m_uncached.find(object); it != m_uncached.end()) {
        auto request = it->second;
        if (request) releaseRequest(request);
        return true;
    }

    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->second.object != object) continue;
        if (it->second.lease) {
            it->second.evictOnRelease = true;
            releaseRequest(it->second.lease);
        } else {
            auto tracked = it->second.object;
            m_cache.erase(it);
            if (tracked) tracked->deleteLater();
        }
        return true;
    }
    return false;
}

auto Pool::genSerial() -> qint64 { return m_serial++; }

auto Pool::toQtMode(IncubationMode mode) const -> QQmlIncubator::IncubationMode {
    switch (mode) {
    case Asynchronous: return QQmlIncubator::Asynchronous;
    case AsynchronousIfNested: return QQmlIncubator::AsynchronousIfNested;
    case Synchronous: return QQmlIncubator::Synchronous;
    }
    return QQmlIncubator::AsynchronousIfNested;
}

} // namespace qml_material

#include "qml_material/util/moc_pool.cpp"
