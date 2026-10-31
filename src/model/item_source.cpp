#include "qml_material/model/item_source.hpp"
#include <QPointer>
#include <QSet>
#include <QThread>
#include <limits>
#include <algorithm>

namespace qml_material
{
ItemSource::ItemSource(QObject* parent)
    : QObject(parent), m_snapshot(std::make_shared<ItemSnapshot>()), m_tail(m_snapshot) {}

ItemPublishResult ItemSource::publish(ItemDataSnapshotPtr data, QVector<ItemChange> changes,
                                      bool reset) {
    if (QThread::currentThread() != thread())
        return { ItemPublishResult::Rejected, QStringLiteral("Publish on the source thread") };
    auto fail = [this](const QString& error) {
        return reject(error);
    };
    if (! data || data->count() < 0) return fail(QStringLiteral("Invalid item count"));
    if (m_tail->revision == std::numeric_limits<quint64>::max())
        return fail(QStringLiteral("Item revision exhausted"));
    if (reset && ! changes.isEmpty()) return fail(QStringLiteral("Reset cannot include changes"));
    const int               count = data->count();
    QVector<ItemDescriptor> descriptors;
    descriptors.reserve(count);
    QSet<QString> keys;
    for (int i = 0; i < count; ++i) {
        auto row = data->descriptorAt(i);
        if (row.key.isEmpty() || keys.contains(row.key) || ! row.contentRevision)
            return fail(QStringLiteral("Invalid item descriptor at row %1").arg(i));
        if (data->indexOfKey(row.key) != std::optional<int>(i))
            return fail(QStringLiteral("Invalid key index at row %1").arg(i));
        const int old =
            m_tail->stableKeys() && data->stableKeys() ? m_tail->indexOfKey(row.key) : -1;
        if (old >= 0 && row.contentRevision < m_tail->descriptorAt(old).contentRevision)
            return fail(QStringLiteral("Item content revision moved backwards"));
        keys.insert(row.key);
        descriptors.append(std::move(row));
    }
    if (! reset) {
        struct Slot {
            int  origin;
            bool updated = false;
        };
        QVector<Slot> positions;
        positions.reserve(m_tail->count());
        for (int i = 0; i < m_tail->count(); ++i) positions.append(Slot { i });
        for (const auto& change : changes) {
            const qint64 end = qint64(change.index) + change.count;
            if (change.index < 0 || change.count <= 0 || change.index > positions.size() ||
                (change.kind != ItemChange::Insert && end > positions.size()) ||
                (change.kind == ItemChange::Move &&
                 (change.destination < 0 || change.destination > positions.size() - change.count)))
                return fail(QStringLiteral("Invalid item change range"));
            switch (change.kind) {
            case ItemChange::Insert:
                if (qint64(positions.size()) + change.count > std::numeric_limits<int>::max())
                    return fail(QStringLiteral("Too many items"));
                positions.insert(change.index, change.count, Slot { -1 });
                break;
            case ItemChange::Remove: positions.remove(change.index, change.count); break;
            case ItemChange::Move: {
                auto moved = positions.mid(change.index, change.count);
                positions.remove(change.index, change.count);
                for (int i = 0; i < moved.size(); ++i)
                    positions.insert(change.destination + i, moved[i]);
                break;
            }
            case ItemChange::Update:
                for (int i = change.index; i < end; ++i) positions[i].updated = true;
                break;
            default: return fail(QStringLiteral("Unknown item change"));
            }
        }
        if (positions.size() != descriptors.size())
            return fail(QStringLiteral("Item change count mismatch"));
        for (int i = 0; i < positions.size(); ++i) {
            if (positions[i].origin < 0) continue;
            const auto& old = m_tail->descriptorAt(positions[i].origin);
            if (old.key != descriptors[i].key || (! positions[i].updated && old != descriptors[i]))
                return fail(QStringLiteral("Unreported item change"));
        }
    }
    ItemSnapshotPtr next(
        new ItemSnapshot(m_tail->revision + 1, std::move(data), std::move(descriptors)));
    return commit(std::move(next), std::move(changes), reset);
}

ItemPublishResult ItemSource::publishValues(QVector<ItemValueUpdate> updates) {
    if (QThread::currentThread() != thread())
        return { ItemPublishResult::Rejected, QStringLiteral("Publish on the source thread") };
    if (updates.isEmpty()) return { ItemPublishResult::Accepted, {} };
    if (m_tail->revision == std::numeric_limits<quint64>::max())
        return reject(QStringLiteral("Item revision exhausted"));
    const auto order = [](const auto& a, const auto& b) {
        return a.index < b.index;
    };
    if (! std::is_sorted(updates.cbegin(), updates.cend(), order))
        std::sort(updates.begin(), updates.end(), order);
    QVector<ItemChange> changes;
    int                 previous = -1;
    for (auto& update : updates) {
        if (update.index < 0 || update.index >= m_tail->count() || update.index == previous)
            return reject(QStringLiteral("Invalid or duplicate item value index"));
        previous       = update.index;
        const auto old = m_tail->descriptorAt(update.index).contentRevision;
        if (old == std::numeric_limits<quint64>::max())
            return reject(QStringLiteral("Item content revision exhausted"));
        if (! update.contentRevision)
            update.contentRevision = std::max(m_tail->revision + 1, old + 1);
        if (update.contentRevision <= old)
            return reject(QStringLiteral("Item value revision must advance"));
        if (! changes.isEmpty() && changes.back().index + changes.back().count == update.index)
            ++changes.back().count;
        else
            changes.append({ ItemChange::Update, update.index, 1 });
    }
    ItemSnapshotPtr next(new ItemSnapshot(m_tail->revision + 1, *m_tail, updates));
    return commit(std::move(next), std::move(changes), false);
}

ItemPublishResult ItemSource::commit(ItemSnapshotPtr next, QVector<ItemChange> changes,
                                     bool reset) {
    m_pending.append(Pending { { m_tail, next, std::move(changes), reset }, {} });
    m_tail = std::move(next);
    drain();
    return { ItemPublishResult::Accepted, {} };
}

ItemPublishResult ItemSource::reject(QString error) {
    ItemPublishResult result { ItemPublishResult::Rejected, error };
    if (QThread::currentThread() != thread()) return result;
    m_pending.append(Pending { {}, std::move(error) });
    drain();
    return result;
}

void ItemSource::drain() {
    if (m_publishing) return;
    m_publishing = true;
    QPointer<ItemSource> guard(this);
    while (! m_pending.isEmpty()) {
        const auto pending = m_pending.takeFirst();
        if (pending.change.after) {
            m_snapshot = pending.change.after;
            m_error.clear();
            Q_EMIT committed(pending.change);
            if (! guard) return;
        } else {
            if (m_error == pending.error) continue;
            m_error = pending.error;
        }
        Q_EMIT stateChanged();
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
    if (! error.isEmpty()) {
        reject(error);
        return;
    }
    publish(makeItemDataSnapshot(std::move(rows), ! m_keyRole.isEmpty(), publicationBase()));
}
} // namespace qml_material
