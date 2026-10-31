#include "qml_material/model/item_model_source.hpp"
#include <QTimer>
#include <QScopeGuard>
#include <algorithm>
#include <iterator>
#include <utility>

namespace qml_material
{
namespace
{
class ModelRoleObject : public QQmlPropertyMap {
public:
    ModelRoleObject(ItemModelSource* source, QString key)
        : QQmlPropertyMap(source), m_source(source), m_key(std::move(key)) {}
    void refresh(const QVariantMap& values) {
        QPointer<ModelRoleObject> guard(this);
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            insert(it.key(), it.value());
            if (! guard) return;
        }
    }

protected:
    QVariant updateValue(const QString& role, const QVariant& value) override {
        if (m_source && m_source->setRole(m_key, role, value)) return value;
        return this->value(role);
    }

private:
    QPointer<ItemModelSource> m_source;
    QString                   m_key;
};
} // namespace
void ItemModelSource::setKeyMode(KeyMode mode) {
    if (m_keyMode == mode) return;
    m_keyMode = mode;
    m_identities.clear();
    QPointer<ItemModelSource> guard(this);
    update();
    if (guard) emit keyModeChanged();
}
bool ItemModelSource::setRole(const QString& key, const QString& role, const QVariant& value) {
    QPointer<ItemModelSource> guard(this);
    flushDataChanges();
    if (! guard) return false;
    const int                    row   = indexOfKey(key);
    QPointer<QAbstractItemModel> model = m_model;
    if (! model || row < 0) return false;
    const auto generation = m_generation;
    const auto names      = model->roleNames();
    if (! guard || ! model || generation != m_generation) return false;
    for (auto it = names.cbegin(); it != names.cend(); ++it)
        if (it.value() == role.toUtf8()) {
            const auto index = model->index(row, 0);
            if (! guard || ! model || generation != m_generation) return false;
            const bool accepted = model->setData(index, value, it.key());
            if (guard) flushDataChanges();
            return accepted;
        }
    return false;
}
QQmlPropertyMap* ItemModelSource::objectForKey(const QString& key) {
    const int row = indexOfKey(key);
    if (row < 0) return nullptr;
    if (m_objects.value(key)) return m_objects.value(key);
    QPointer<ItemModelSource> guard(this);
    QPointer<ModelRoleObject> object = new ModelRoleObject(this, key);
    m_objects.insert(key, object);
    object->refresh(valueAt(row).toMap());
    if (! guard || ! object) return nullptr;
    connect(this, &ItemSource::stateChanged, object, [this, object, key] {
        const int index = indexOfKey(key);
        if (index >= 0) object->refresh(valueAt(index).toMap());
    });
    return object;
}

void ItemModelSource::setModel(QAbstractItemModel* model) {
    if (m_model == model) return;
    cancelDataChanges();
    for (const auto& connection : m_connections) disconnect(connection);
    m_connections.clear();
    m_model = model;
    m_identities.clear();
    ++m_modelGeneration;
    m_fetchQueued = false;
    m_requestedCount.reset();
    m_demand = {};
    if (model) {
        m_connections.append({ connect(model,
                                       &QAbstractItemModel::rowsAboutToBeInserted,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::rowsAboutToBeRemoved,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::rowsAboutToBeMoved,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::layoutAboutToBeChanged,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::modelAboutToBeReset,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::columnsAboutToBeInserted,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::columnsAboutToBeRemoved,
                                       this,
                                       &ItemModelSource::flushDataChanges),
                               connect(model,
                                       &QAbstractItemModel::columnsAboutToBeMoved,
                                       this,
                                       &ItemModelSource::flushDataChanges) });
        m_connections.append(
            connect(model,
                    &QAbstractItemModel::rowsInserted,
                    this,
                    [this](const QModelIndex& parent, int first, int last) {
                        if (! parent.isValid())
                            update({ { ItemChange::Insert, first, last - first + 1 } }, false);
                    }));
        m_connections.append(
            connect(model,
                    &QAbstractItemModel::rowsRemoved,
                    this,
                    [this](const QModelIndex& parent, int first, int last) {
                        if (! parent.isValid())
                            update({ { ItemChange::Remove, first, last - first + 1 } }, false);
                    }));
        m_connections.append(
            connect(model,
                    &QAbstractItemModel::rowsMoved,
                    this,
                    [this](const QModelIndex& from,
                           int                first,
                           int                last,
                           const QModelIndex& to,
                           int                destination) {
                        if (from.isValid() && to.isValid()) return;
                        if (from.isValid() || to.isValid()) {
                            update();
                            return;
                        }
                        const int count = last - first + 1;
                        update({ { ItemChange::Move,
                                   first,
                                   count,
                                   destination > first ? destination - count : destination } },
                               false);
                    }));
        m_connections.append(
            connect(model,
                    &QAbstractItemModel::dataChanged,
                    this,
                    [this](const QModelIndex& first, const QModelIndex& last, const QList<int>&) {
                        if (! first.parent().isValid() && first.column() == 0)
                            dataChanged(first.row(), last.row());
                    }));
        m_connections.append(connect(model, &QAbstractItemModel::modelReset, this, [this] {
            update();
        }));
        m_connections.append(connect(model, &QAbstractItemModel::layoutChanged, this, [this] {
            update();
        }));
        m_connections.append(connect(model, &QAbstractItemModel::columnsInserted, this, [this] {
            update();
        }));
        m_connections.append(connect(model, &QAbstractItemModel::columnsRemoved, this, [this] {
            update();
        }));
        m_connections.append(connect(model, &QAbstractItemModel::columnsMoved, this, [this] {
            update();
        }));
        m_connections.append(connect(model, &QObject::destroyed, this, [this] {
            ++m_modelGeneration;
            m_fetchQueued = false;
            m_model       = nullptr;
            m_requestedCount.reset();
            m_demand = {};
            QPointer<ItemModelSource> guard(this);
            update();
            if (guard) Q_EMIT modelChanged();
            if (guard) Q_EMIT fetchStateChanged();
        }));
    }
    QPointer<ItemModelSource> guard(this);
    update();
    if (guard) Q_EMIT modelChanged();
    if (guard) Q_EMIT fetchStateChanged();
}
void ItemModelSource::setKeyRole(const QString& role) {
    if (m_keyRole == role) return;
    m_keyRole = role;
    QPointer<ItemModelSource> guard(this);
    update();
    if (guard) Q_EMIT keyRoleChanged();
}
void ItemModelSource::setTypeRole(const QString& role) {
    if (m_typeRole == role) return;
    m_typeRole = role;
    QPointer<ItemModelSource> guard(this);
    update();
    if (guard) Q_EMIT typeRoleChanged();
}
void ItemModelSource::setRoles(const QStringList& roles) {
    if (m_roles == roles) return;
    m_roles = roles;
    QPointer<ItemModelSource> guard(this);
    update();
    if (guard) Q_EMIT rolesChanged();
}

void ItemModelSource::setDeferDataChanges(bool defer) {
    if (m_deferDataChanges == defer) return;
    m_deferDataChanges = defer;
    QPointer<ItemModelSource> guard(this);
    if (! defer) flushDataChanges();
    if (guard) emit deferDataChangesChanged();
}
void ItemModelSource::cancelDataChanges() {
    ++m_dataChangeTicket;
    m_dataChangeQueued = false;
    m_dirtyRanges.clear();
}
void ItemModelSource::dataChanged(int first, int last) {
    if (! m_deferDataChanges) {
        update({ { ItemChange::Update, first, last - first + 1 } }, false);
        return;
    }
    if (first < 0 || last < first) return;
    if (m_preparing) {
        // A notification during model reads invalidates the partially prepared projection.
        ++m_generation;
        m_resetRequired = true;
    }
    auto it = m_dirtyRanges.lowerBound(first);
    if (it != m_dirtyRanges.begin()) {
        auto previous = std::prev(it);
        if (qint64(previous.value()) + 1 >= first) it = previous;
    }
    while (it != m_dirtyRanges.end() && it.key() <= qint64(last) + 1) {
        first = std::min(first, it.key());
        last  = std::max(last, it.value());
        it    = m_dirtyRanges.erase(it);
    }
    m_dirtyRanges.insert(first, last);
    if (m_dataChangeQueued) return;
    m_dataChangeQueued = true;
    const auto ticket  = ++m_dataChangeTicket;
    QTimer::singleShot(0, this, [this, ticket] {
        if (ticket == m_dataChangeTicket) flushDataChanges();
    });
}
void ItemModelSource::flushDataChanges() {
    if (m_dirtyRanges.isEmpty()) return;
    const auto ranges = std::exchange(m_dirtyRanges, {});
    ++m_dataChangeTicket;
    m_dataChangeQueued = false;
    QVector<ItemChange> changes;
    changes.reserve(ranges.size());
    for (auto it = ranges.cbegin(); it != ranges.cend(); ++it)
        changes.append({ ItemChange::Update, it.key(), it.value() - it.key() + 1 });
    update(std::move(changes), false);
}

void ItemModelSource::update(QVector<ItemChange> changes, bool reset) {
    if (reset) cancelDataChanges();
    if (reset) m_requestedCount.reset();
    const auto                   generation = ++m_generation;
    QPointer<ItemModelSource>    guard(this);
    const bool                   preparing   = std::exchange(m_preparing, true);
    const auto                   preparation = qScopeGuard([guard, preparing] {
        if (guard) guard->m_preparing = preparing;
    });
    QPointer<QAbstractItemModel> model       = m_model;
    if (! model) {
        m_rows.clear();
        m_resetRequired = false;
        publish(makeItemDataSnapshot({}, false, publicationBase()));
        return;
    }
    const bool persistent = m_keyMode == PersistentIndexKey;
    const auto keyRole    = m_keyRole;
    const auto typeRole   = m_typeRole;
    const auto projection = m_roles;
    const auto before     = publicationBase();
    const auto current    = [&] {
        return guard && model && guard->m_generation == generation;
    };
    auto names = model->roleNames();
    if (! current()) return;
    int                keyId = -1, typeId = -1;
    QMap<int, QString> fields;
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        const auto name = QString::fromUtf8(it.value());
        if (name == keyRole) keyId = it.key();
        if (name == typeRole) typeId = it.key();
        if (projection.isEmpty() || projection.contains(name)) fields.insert(it.key(), name);
    }
    QString error;
    if (! persistent && (keyRole.isEmpty() || keyId < 0))
        error = QStringLiteral("A valid string keyRole is required");
    if (! typeRole.isEmpty() && typeId < 0) error = QStringLiteral("Unknown typeRole");
    for (const auto& field : projection)
        if (! fields.values().contains(field))
            error = QStringLiteral("Unknown projected role: %1").arg(field);
    const int count = model->rowCount();
    if (! current()) return;
    if (count < 0) error = QStringLiteral("Invalid model row count");
    if (! error.isEmpty()) {
        m_resetRequired = true;
        reject(error);
        return;
    }
    reset                         = reset || m_resetRequired;
    QVector<ItemRecord>      rows = reset ? QVector<ItemRecord>(count) : m_rows;
    QVector<QPair<int, int>> reads;
    if (reset)
        reads.append({ 0, count });
    else {
        for (const auto& change : changes) {
            const auto end = qint64(change.index) + change.count;
            if (change.index < 0 || change.count <= 0 || change.index > rows.size() ||
                (change.kind != ItemChange::Insert && end > rows.size()) ||
                (change.kind == ItemChange::Move &&
                 (change.destination < 0 || change.destination > rows.size() - change.count))) {
                m_resetRequired = true;
                reject(QStringLiteral("Invalid model change range"));
                return;
            }
            switch (change.kind) {
            case ItemChange::Insert:
                rows.insert(change.index, change.count, ItemRecord {});
                reads.append({ change.index, change.count });
                break;
            case ItemChange::Remove: rows.remove(change.index, change.count); break;
            case ItemChange::Move: {
                const auto moved = rows.mid(change.index, change.count);
                rows.remove(change.index, change.count);
                for (int i = 0; i < moved.size(); ++i)
                    rows.insert(change.destination + i, moved[i]);
                break;
            }
            case ItemChange::Update: reads.append({ change.index, change.count }); break;
            }
        }
        if (rows.size() != count) {
            m_resetRequired = true;
            reject(QStringLiteral("Model change count mismatch"));
            return;
        }
    }
    QHash<int, QString> rowKeys;
    auto                identities = m_identities;
    if (persistent) {
        identities.removeIf([](const auto& entry) {
            return ! entry.first.isValid();
        });
        for (const auto& entry : identities) rowKeys.insert(entry.first.row(), entry.second);
    }
    for (const auto& [first, length] : reads) {
        for (int row = first; row < first + length; ++row) {
            const auto index = model->index(row, 0);
            if (! current()) return;
            QVariantMap value;
            QVariant    key, type;
            // Query each requested role once, including identity roles outside the projection.
            auto requested = fields;
            if (! persistent) requested.insert(keyId, QString::fromUtf8(names.value(keyId)));
            if (typeId >= 0) requested.insert(typeId, QString::fromUtf8(names.value(typeId)));
            for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
                const auto data = model->data(index, it.key());
                if (! current()) return;
                if (it.key() == keyId) key = data;
                if (it.key() == typeId) type = data;
                if (fields.contains(it.key())) value.insert(it.value(), data);
            }
            if (persistent) {
                key = rowKeys.value(row);
                if (key.toString().isEmpty()) {
                    key = QStringLiteral("model:%1").arg(++m_nextIdentity);
                    identities.append({ QPersistentModelIndex(index), key.toString() });
                }
            }
            if (key.metaType().id() != QMetaType::QString ||
                (typeId >= 0 && type.metaType().id() != QMetaType::QString)) {
                m_resetRequired = true;
                reject(QStringLiteral("Key and reuse type must be strings"));
                return;
            }
            if (rows[row].key != key.toString() && ! rows[row].key.isEmpty()) reset = true;
            rows[row] = { key.toString(), type.toString(), value, before->revision + 1 };
        }
    }
    if (persistent) m_identities = std::move(identities);
    m_rows            = rows;
    m_resetRequired   = false;
    const auto result = publish(makeItemDataSnapshot(std::move(rows), true, before),
                                reset ? QVector<ItemChange>() : changes,
                                reset);
    if (current() && ! result) m_resetRequired = true;
}

void ItemModelSource::requestMore(const ItemDemand& demand) {
    if (! demand.valid() || demand.snapshot() != snapshot() || ! demand.reachesEnd()) return;
    m_demand = demand;
    if (m_fetchQueued || ! m_model || m_requestedCount == count()) return;
    m_fetchQueued         = true;
    const auto generation = m_modelGeneration;
    QTimer::singleShot(0, this, [this, generation] {
        if (generation != m_modelGeneration) return;
        m_fetchQueued     = false;
        const auto demand = m_demand;
        if (! demand.valid() || demand.snapshot() != snapshot() || ! demand.reachesEnd() ||
            m_requestedCount == count())
            return;
        QPointer<ItemModelSource>    guard(this);
        QPointer<QAbstractItemModel> model    = m_model;
        const bool                   canFetch = model && model->canFetchMore({});
        if (! guard || generation != m_modelGeneration || ! demand.valid() ||
            demand.snapshot() != snapshot())
            return;
        if (canFetch && model) {
            m_requestedCount = count();
            model->fetchMore({});
        }
    });
}
void ItemModelSource::notifyFetchStateChanged() {
    m_requestedCount.reset();
    QPointer<ItemModelSource> guard(this);
    Q_EMIT fetchStateChanged();
    if (guard) requestMore(m_demand);
}
} // namespace qml_material
