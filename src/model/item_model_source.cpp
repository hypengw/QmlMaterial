#include "qml_material/model/item_model_source.hpp"
#include <QTimer>

namespace qml_material
{
void ItemModelSource::setModel(QAbstractItemModel* model) {
    if (m_model == model) return;
    for (const auto& connection : m_connections) disconnect(connection);
    m_connections.clear();
    m_model = model;
    ++m_modelGeneration;
    m_fetchQueued = false;
    if (model) {
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
        m_connections.append(connect(
            model,
            &QAbstractItemModel::dataChanged,
            this,
            [this](const QModelIndex& first, const QModelIndex& last, const QList<int>&) {
                if (! first.parent().isValid() && first.column() == 0)
                    update({ { ItemChange::Update, first.row(), last.row() - first.row() + 1 } },
                           false);
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

void ItemModelSource::update(QVector<ItemChange> changes, bool reset) {
    const auto                   generation = ++m_generation;
    QPointer<ItemModelSource>    guard(this);
    QPointer<QAbstractItemModel> model = m_model;
    if (! model) {
        publish({}, false);
        return;
    }
    const auto keyRole    = m_keyRole;
    const auto typeRole   = m_typeRole;
    const auto projection = m_roles;
    const auto before     = snapshot();
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
    if (keyRole.isEmpty() || keyId < 0)
        error = QStringLiteral("A valid string keyRole is required");
    if (! typeRole.isEmpty() && typeId < 0) error = QStringLiteral("Unknown typeRole");
    for (const auto& field : projection)
        if (! fields.values().contains(field))
            error = QStringLiteral("Unknown projected role: %1").arg(field);
    const int count = model->rowCount();
    if (! current()) return;
    if (count < 0) error = QStringLiteral("Invalid model row count");
    if (! error.isEmpty()) {
        publish({}, true, {}, true, error);
        return;
    }
    reset                         = reset || ! before->error.isEmpty();
    QVector<ItemRecord>      rows = reset ? QVector<ItemRecord>(count) : before->rows;
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
                publish({}, true, {}, true, QStringLiteral("Invalid model change range"));
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
            publish({}, true, {}, true, QStringLiteral("Model change count mismatch"));
            return;
        }
    }
    for (const auto& [first, length] : reads) {
        for (int row = first; row < first + length; ++row) {
            const auto index = model->index(row, 0);
            if (! current()) return;
            QVariantMap value;
            QVariant    key, type;
            // Query each requested role once, including identity roles outside the projection.
            auto requested = fields;
            requested.insert(keyId, QString::fromUtf8(names.value(keyId)));
            if (typeId >= 0) requested.insert(typeId, QString::fromUtf8(names.value(typeId)));
            for (auto it = requested.cbegin(); it != requested.cend(); ++it) {
                const auto data = model->data(index, it.key());
                if (! current()) return;
                if (it.key() == keyId) key = data;
                if (it.key() == typeId) type = data;
                if (fields.contains(it.key())) value.insert(it.value(), data);
            }
            if (key.metaType().id() != QMetaType::QString ||
                (typeId >= 0 && type.metaType().id() != QMetaType::QString)) {
                publish({}, true, {}, true, QStringLiteral("Key and reuse type must be strings"));
                return;
            }
            rows[row] = { key.toString(), type.toString(), value, before->revision + 1 };
        }
    }
    publish(std::move(rows), true, reset ? QVector<ItemChange>() : changes, reset);
}

void ItemModelSource::requestMore() {
    if (m_fetchQueued || ! m_model) return;
    m_fetchQueued         = true;
    const auto generation = m_modelGeneration;
    QTimer::singleShot(0, this, [this, generation] {
        if (generation != m_modelGeneration) return;
        QPointer<ItemModelSource>    guard(this);
        QPointer<QAbstractItemModel> model    = m_model;
        const bool                   canFetch = model && model->canFetchMore({});
        if (! guard || generation != m_modelGeneration) return;
        if (canFetch && model) model->fetchMore({});
        if (guard && generation == m_modelGeneration) m_fetchQueued = false;
    });
}
void ItemModelSource::notifyFetchStateChanged() { Q_EMIT fetchStateChanged(); }
} // namespace qml_material
