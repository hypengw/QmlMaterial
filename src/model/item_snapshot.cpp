#include "qml_material/model/item_snapshot.hpp"
#include <QHash>
#include <algorithm>
#include <limits>

namespace qml_material
{
ItemDataSnapshot::~ItemDataSnapshot() = default;
ItemSnapshot::ItemSnapshot(): revision(0) {}
ItemSnapshot::ItemSnapshot(quint64 version, ItemDataSnapshotPtr data,
                           QVector<ItemDescriptor> descriptors)
    : revision(version), m_data(std::move(data)), m_descriptors(std::move(descriptors)) {}

int ItemSnapshot::indexOfKey(const QString& key) const {
    if (! m_data) return -1;
    const auto index = m_data->indexOfKey(key);
    return index && *index >= 0 && *index < count() && descriptorAt(*index).key == key ? *index
                                                                                       : -1;
}
QVariant ItemSnapshot::valueAt(int index) const {
    return index >= 0 && index < count() ? m_data->valueAt(index) : QVariant();
}
ItemRecord ItemSnapshot::recordAt(int index) const {
    const auto& descriptor = descriptorAt(index);
    return { descriptor.key, descriptor.reuseType, valueAt(index), descriptor.contentRevision };
}

namespace
{
class VectorItemData final : public ItemDataSnapshot {
public:
    VectorItemData(QVector<ItemRecord> rows, bool stable, const ItemSnapshotPtr& before)
        : m_rows(std::move(rows)), m_stable(stable) {
        const quint64 nextRevision = before ? before->revision + 1 : 1;
        for (int i = 0; i < m_rows.size(); ++i) {
            auto& row = m_rows[i];
            if (! stable) row.key = QStringLiteral("%1:%2").arg(nextRevision).arg(i);
            m_indexes.insert(row.key, i);
            if (row.contentRevision) continue;
            const int oldIndex =
                stable && before && before->stableKeys() ? before->indexOfKey(row.key) : -1;
            if (oldIndex >= 0) {
                const auto& old = before->descriptorAt(oldIndex);
                if (old.reuseType == row.reuseType && before->valueAt(oldIndex) == row.value)
                    row.contentRevision = old.contentRevision;
                else if (old.contentRevision < std::numeric_limits<quint64>::max())
                    row.contentRevision = std::max(nextRevision, old.contentRevision + 1);
            } else {
                row.contentRevision = nextRevision;
            }
        }
    }
    int count() const override {
        return m_rows.size() <= std::numeric_limits<int>::max() ? int(m_rows.size()) : -1;
    }
    bool           stableKeys() const override { return m_stable; }
    ItemDescriptor descriptorAt(int index) const override {
        const auto& row = m_rows.at(index);
        return { row.key, row.reuseType, row.contentRevision };
    }
    std::optional<int> indexOfKey(const QString& key) const override {
        const auto found = m_indexes.constFind(key);
        return found == m_indexes.cend() ? std::nullopt : std::optional<int>(*found);
    }
    QVariant valueAt(int index) const override { return m_rows.at(index).value; }

private:
    QVector<ItemRecord> m_rows;
    QHash<QString, int> m_indexes;
    bool                m_stable;
};
} // namespace

ItemDataSnapshotPtr makeItemDataSnapshot(QVector<ItemRecord> rows, bool stableKeys,
                                         const ItemSnapshotPtr& before) {
    return std::make_shared<VectorItemData>(std::move(rows), stableKeys, before);
}
} // namespace qml_material
