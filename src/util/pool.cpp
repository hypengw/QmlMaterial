#include "qml_material/util/pool.hpp"
#include "qml_material/util/loggingcategory.hpp"

namespace
{
auto variant_map_to_string(const QVariantMap& m) -> QString {
    QString out;
    for (const auto& el : m.asKeyValueRange()) {
        out.append(QString("%1%2").arg(el.first).arg(el.second.toString()));
    }
    return out;
}
} // namespace

namespace qml_material
{
PoolIncubator::PoolIncubator(Pool* p, qint64 id, IncubationMode mode)
    : QQmlIncubator(mode), m_pool(p), m_id(id) {}
void PoolIncubator::statusChanged(Status s) {
    QQmlIncubator::statusChanged(s);
    m_pool->incubatorStateChanged(m_id, s);
}
void PoolIncubator::setInitialState(QObject* o) {
    QQmlIncubator::setInitialState(o);
    m_pool->setInitialState(o);
}

Pool::Pool(QObject* parent): QObject(parent), m_serial(0), m_async(true) {
    connect(this, &Pool::queueAdded, this, &Pool::onQueueAdded, Qt::QueuedConnection);
}
Pool::~Pool() {
    for (const auto& el : m_objs) {
        if (el.second) {
            delete el.second.get();
        }
    }
    for (auto el : m_uncache_objs) {
        delete el;
    }
    for (const auto& el : m_tasks) {
        if (el.second.object) {
            delete el.second.object;
        }
    }
}
void Pool::incubatorStateChanged(qint64 id, QQmlIncubator::Status status) {
    if (status == QQmlIncubator::Loading || status == QQmlIncubator::Null) return;

    auto it = m_tasks.find(id);
    if (it == m_tasks.end()) return;
    auto& t         = it->second;
    auto  incubator = it->second.incubator;

    if (status == QQmlIncubator::Ready) {
        auto object = incubator->object();
        incubator->clear();
        if (t.cache) {
            m_objs.insert({ t.key, object });
        }
        t.object = object;
        queueAdded(id);
    } else if (status == QQmlIncubator::Error) {
        if (! incubator->errors().isEmpty()) {
            qCWarning(qml_material_logcat()) << incubator->errors();
        }
        delete incubator->object();
        clearTask(it);
    }
}
void Pool::setInitialState(QObject*) {}

void Pool::clearTask(task_iterator it) {
    auto& t = it->second;
    if (t.incubator) {
        t.incubator->clear();
        delete t.incubator;
        t.incubator = nullptr;
    }
    if (t.component) {
        if (t.component->parent() == this) {
            t.component->deleteLater();
            t.component = nullptr;
        }
    }
    m_tasks.erase(it);
}

auto Pool::genSerial() const -> qint64 { return m_serial++; }

void Pool::addWithKey(const QString& key, QQmlComponent* component, const QVariantMap& props,
                      bool cache) {
    if (tryCache(key, cache)) return;
    if (! component) return;
    auto id   = genSerial();
    auto task = Task {
        .key       = key,
        .cache     = cache,
        .object    = nullptr,
        .component = component,
        .incubator = new PoolIncubator(
            this, id, async() ? QQmlIncubator::Asynchronous : QQmlIncubator::Synchronous),
    };
    m_tasks.insert({ id, task });
    task.incubator->setInitialProperties(props);

    if (! component->isLoading()) {
        onComponentLoaded(id);
    } else {
        QObject::connect(component, &QQmlComponent::statusChanged, this, [this, id]() {
            onComponentLoaded(id);
        });
        QObject::connect(component, &QQmlComponent::progressChanged, this, [this, id](qreal p) {
            onComponentProgress(id, p);
        });
    }
}
void Pool::addWithKey(const QString& key, const QUrl& url, const QVariantMap& props, bool cache) {
    if (tryCache(key, cache)) return;

    auto component = createComponent();
    component->loadUrl(url,
                       async() ? QQmlComponent::Asynchronous : QQmlComponent::PreferSynchronous);
    addWithKey(key, component, props, cache);
}
void Pool::addFromModuleWithKey(const QString& key, const QString& uri, const QString& typeName,
                                const QVariantMap& props, bool cache) {
    if (tryCache(key, cache)) return;

    auto component = createComponent();
    component->loadFromModule(
        uri, typeName, async() ? QQmlComponent::Asynchronous : QQmlComponent::PreferSynchronous);
    addWithKey(key, component, props, cache);
}
auto Pool::createComponent() -> QQmlComponent* {
    auto engine = qmlEngine(this);
    if (engine == nullptr) return nullptr;
    return new QQmlComponent(engine, this);
}
void Pool::add(QQmlComponent* component, const QVariantMap& props, bool cache) {
    QString key = QString("%1%2")
                      .arg(QString::number((std::ptrdiff_t)component))
                      .arg(variant_map_to_string(props));
    addWithKey(key, component, props, cache);
}
void Pool::add(const QUrl& url, const QVariantMap& props, bool cache) {
    QString key = QString("%1%2").arg(url.toString()).arg(variant_map_to_string(props));
    addWithKey(key, url, props, cache);
}
void Pool::addFromModule(const QString& uri, const QString& typeName, const QVariantMap& props,
                         bool cache) {
    QString key = QString("%1%2%3").arg(uri).arg(typeName).arg(variant_map_to_string(props));
    addFromModuleWithKey(key, uri, typeName, props, cache);
}
void Pool::onComponentProgress(qint64, qreal) {}
void Pool::onComponentLoaded(qint64 id) {
    if (auto it = m_tasks.find(id); it != m_tasks.end()) {
        auto& t = it->second;
        if (t.component->isError()) {
            qCCritical(qml_material_logcat()) << t.component->errorString();
        } else {
            t.component->create(*t.incubator);
        }
    }
}
void Pool::onQueueAdded(qint64 id) {
    if (m_tasks.empty()) return;
    auto it = m_tasks.begin();
    if (it->first != id) return;

    auto& t = it->second;
    if (t.object) {
        if (! t.cache) {
            connect(t.object, &QObject::destroyed, this, &Pool::onUncacheObjectDeleted);
            m_uncache_objs.insert(t.object);
        }

        objectAdded(t.object, t.cache);
        clearTask(it);

        // try next
        if (! m_tasks.empty()) {
            auto it = m_tasks.begin();
            queueAdded(it->first);
        }
    }
}
void Pool::onUncacheObjectDeleted(QObject* o) { m_uncache_objs.erase(o); }

auto Pool::async() const -> bool { return m_async; }
void Pool::setAsync(bool v) {
    if (m_async != v) {
        m_async = v;
        asyncChanged(m_async);
    }
}

bool Pool::removeObject(QObject* o) {
    if (m_uncache_objs.erase(o) > 0) {
        o->deleteLater();
        return true;
    }
    for (auto& el : m_objs) {
        if (el.second == o) {
            o->deleteLater();
            m_objs.erase(el.first);
            return true;
        }
    }
    return false;
}

auto Pool::tryCache(QStringView key, bool cache) -> bool {
    if (! cache) return false;
    if (auto it = m_objs.find(key); it != m_objs.end()) {
        auto id = genSerial();
        m_tasks.insert({ id,
                         Task {
                             .key    = key.toString(),
                             .cache  = true,
                             .object = it->second,
                         } });
        queueAdded(id);
        return true;
    }
    return false;
}

} // namespace qml_material

#include "qml_material/util/moc_pool.cpp"