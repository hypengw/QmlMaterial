#include "qml_material/model/item_source.hpp"
#include <QPointer>
#include <limits>

namespace qml_material
{
ItemSource::ItemSource(QObject* parent)
    : QObject(parent), m_snapshot(std::make_shared<ItemSnapshot>()) {}

void ItemSource::requestMore() {}

void ItemSource::publish(QVector<ItemRecord> rows, bool stableKeys, QVector<ItemChange> changes,
                         bool reset, QString error) {
    auto next        = std::make_shared<ItemSnapshot>();
    next->revision   = revision() + 1;
    next->stableKeys = stableKeys;
    if (rows.size() > std::numeric_limits<int>::max()) error = QStringLiteral("Too many rows");
    if (! reset && error.isEmpty()) {
        qint64 expected = count();
        for (const auto& change : changes) {
            const qint64 end = qint64(change.index) + change.count;
            if (change.index < 0 || change.count <= 0 || change.index > expected ||
                (change.kind != ItemChange::Insert && end > expected) ||
                (change.kind == ItemChange::Move &&
                 (change.destination < 0 || change.destination > expected - change.count))) {
                error = QStringLiteral("Invalid item change range");
                break;
            }
            if (change.kind == ItemChange::Insert) expected += change.count;
            if (change.kind == ItemChange::Remove) expected -= change.count;
        }
        if (expected != rows.size()) error = QStringLiteral("Item change count mismatch");
    }
    if (error.isEmpty()) {
        for (int i = 0; i < rows.size(); ++i) {
            if (! stableKeys) rows[i].key = QStringLiteral("%1:%2").arg(next->revision).arg(i);
            const auto& key = rows[i].key;
            if (key.isEmpty() || next->indexes.contains(key)) {
                error = QStringLiteral("Missing or duplicate item key at row %1").arg(i);
                break;
            }
            next->indexes.insert(key, i);
            if (! rows[i].contentRevision) {
                const int oldIndex =
                    stableKeys && m_snapshot->stableKeys ? m_snapshot->indexOfKey(key) : -1;
                if (oldIndex >= 0 && m_snapshot->rows[oldIndex].value == rows[i].value &&
                    m_snapshot->rows[oldIndex].reuseType == rows[i].reuseType)
                    rows[i].contentRevision = m_snapshot->rows[oldIndex].contentRevision;
                else
                    rows[i].contentRevision = next->revision;
            }
        }
    }
    next->error = std::move(error);
    if (next->error.isEmpty())
        next->rows = std::move(rows);
    else {
        next->indexes.clear();
        reset = true;
        changes.clear();
    }
    m_pending.append({ m_snapshot, next, std::move(changes), reset });
    m_snapshot = next;
    if (m_publishing) return;
    m_publishing = true;
    QPointer<ItemSource> guard(this);
    while (! m_pending.isEmpty()) {
        const auto change = m_pending.takeFirst();
        Q_EMIT committed(change);
        if (! guard) return;
        if (m_pending.isEmpty()) Q_EMIT stateChanged();
        if (! guard) return;
    }
    m_publishing = false;
}

void ListSnapshotSource::setItems(const QVariantList& items) {
    if (m_items == items) return;
    m_items = items;
    QPointer<ListSnapshotSource> guard(this);
    rebuild();
    if (guard) Q_EMIT itemsChanged();
}
void ListSnapshotSource::setKeyRole(const QString& role) {
    if (m_keyRole == role) return;
    m_keyRole = role;
    QPointer<ListSnapshotSource> guard(this);
    rebuild();
    if (guard) Q_EMIT keyRoleChanged();
}
void ListSnapshotSource::setTypeRole(const QString& role) {
    if (m_typeRole == role) return;
    m_typeRole = role;
    QPointer<ListSnapshotSource> guard(this);
    rebuild();
    if (guard) Q_EMIT typeRoleChanged();
}
void ListSnapshotSource::rebuild() {
    QVector<ItemRecord> rows;
    rows.reserve(m_items.size());
    QString error;
    for (const auto& value : m_items) {
        const auto map  = value.toMap();
        const auto key  = map.value(m_keyRole);
        const auto type = map.value(m_typeRole);
        if (! m_keyRole.isEmpty() && key.metaType().id() != QMetaType::QString)
            error = QStringLiteral("keyRole must name a string field");
        if (! m_typeRole.isEmpty() && type.metaType().id() != QMetaType::QString)
            error = QStringLiteral("typeRole must name a string field");
        rows.append({ key.toString(), type.toString(), value });
    }
    publish(std::move(rows), ! m_keyRole.isEmpty(), {}, true, error);
}
} // namespace qml_material
