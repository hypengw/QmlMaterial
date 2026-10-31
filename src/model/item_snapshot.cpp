#include "qml_material/model/item_snapshot.hpp"
#include <QHash>
#include <algorithm>
#include <array>
#include <limits>

namespace qml_material
{
ItemDataSnapshot::~ItemDataSnapshot() = default;
class ItemSnapshotRows {
public:
    struct Row {
        ItemDescriptor descriptor;
        QVariant       value;
    };
    static constexpr int pageSize = 32;
    using Page                    = std::array<std::shared_ptr<const Row>, pageSize>;
    explicit ItemSnapshotRows(QVector<ItemDescriptor> descriptors)
        : base(std::make_shared<const QVector<ItemDescriptor>>(std::move(descriptors))),
          pages((base->size() + pageSize - 1) / pageSize) {}
    int        count() const { return int(base->size()); }
    const Row* rowAt(int index) const {
        const auto& page = pages.at(index / pageSize);
        return page ? (*page)[index % pageSize].get() : nullptr;
    }
    const ItemDescriptor& descriptorAt(int index) const {
        if (auto row = rowAt(index)) return row->descriptor;
        return base->at(index);
    }
    std::shared_ptr<const ItemSnapshotRows> updated(const QVector<ItemValueUpdate>& updates) const {
        auto                  result    = std::make_shared<ItemSnapshotRows>(*this);
        int                   pageIndex = -1;
        std::shared_ptr<Page> page;
        for (const auto& update : updates) {
            const int nextPage = update.index / pageSize;
            if (nextPage != pageIndex) {
                pageIndex            = nextPage;
                const auto& previous = pages.at(pageIndex);
                page = previous ? std::make_shared<Page>(*previous) : std::make_shared<Page>();
                result->pages[pageIndex] = page;
            }
            const auto& old                  = descriptorAt(update.index);
            (*page)[update.index % pageSize] = std::make_shared<const Row>(
                Row { { old.key, update.reuseType.value_or(old.reuseType), update.contentRevision },
                      update.value });
        }
        return result;
    }

private:
    std::shared_ptr<const QVector<ItemDescriptor>> base;
    QVector<std::shared_ptr<const Page>>           pages;
};

ItemSnapshot::ItemSnapshot(): revision(0) {}
ItemSnapshot::ItemSnapshot(quint64 version, ItemDataSnapshotPtr data,
                           QVector<ItemDescriptor> descriptors)
    : revision(version),
      m_data(std::move(data)),
      m_rows(std::make_shared<ItemSnapshotRows>(std::move(descriptors))),
      m_count(m_rows->count()) {}
ItemSnapshot::ItemSnapshot(quint64 version, const ItemSnapshot& before,
                           const QVector<ItemValueUpdate>& updates)
    : revision(version),
      m_data(before.m_data),
      m_rows(before.m_rows->updated(updates)),
      m_count(before.m_count) {}

const ItemDescriptor& ItemSnapshot::descriptorAt(int index) const {
    return m_rows->descriptorAt(index);
}

int ItemSnapshot::indexOfKey(const QString& key) const {
    if (! m_data) return -1;
    const auto index = m_data->indexOfKey(key);
    return index && *index >= 0 && *index < count() && descriptorAt(*index).key == key ? *index
                                                                                       : -1;
}
QVariant ItemSnapshot::valueAt(int index) const {
    if (index < 0 || index >= count()) return {};
    if (auto row = m_rows->rowAt(index)) return row->value;
    return m_data->valueAt(index);
}
bool ItemSnapshot::sharesValueAt(int index, const ItemSnapshot& other, int otherIndex) const {
    if (index < 0 || index >= count() || otherIndex < 0 || otherIndex >= other.count())
        return false;
    const auto row      = m_rows->rowAt(index);
    const auto previous = other.m_rows->rowAt(otherIndex);
    if (row || previous) return row && row == previous;
    return m_data == other.m_data && index == otherIndex;
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
