#pragma once

#include <QtQml/qqmlregistration.h>
#include "qml_material/model/item_snapshot.hpp"
#include <algorithm>
#include <limits>

namespace qml_material
{
class QML_MATERIAL_API ItemDemand {
    Q_GADGET
    QML_VALUE_TYPE(itemDemand)
    Q_PROPERTY(bool valid READ valid)
    Q_PROPERTY(int first READ first CONSTANT)
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(QVector<int> indices READ indices CONSTANT)
    Q_PROPERTY(quint64 revision READ revision CONSTANT)
    Q_PROPERTY(bool reachesEnd READ reachesEnd CONSTANT)
public:
    ItemDemand() = default;
    ItemDemand(ItemSnapshotPtr snapshot, int first, int count, bool reachesEnd = false)
        : m_snapshot(std::move(snapshot)), m_first(first), m_count(count), m_end(reachesEnd) {}
    ItemDemand(ItemSnapshotPtr snapshot, QVector<int> indices, bool reachesEnd = false)
        : m_snapshot(std::move(snapshot)), m_indices(std::move(indices)), m_end(reachesEnd) {
        std::sort(m_indices.begin(), m_indices.end());
        m_indices.erase(std::unique(m_indices.begin(), m_indices.end()), m_indices.end());
        if (! m_indices.isEmpty()) {
            m_first           = m_indices.first();
            const auto length = qint64(m_indices.last()) - m_first + 1;
            m_count           = length <= std::numeric_limits<int>::max() ? int(length) : -1;
        }
    }
    ItemSnapshotPtr snapshot() const { return m_snapshot; }
    int             first() const { return m_first; }
    int             count() const { return m_count; }
    QVector<int>    indices() const {
        QVector<int> result;
        if (! valid()) return result;
        if (! m_indices.isEmpty()) return m_indices;
        result.reserve(m_count);
        for (int i = 0; i < m_count; ++i) result.append(m_first + i);
        return result;
    }
    quint64 revision() const { return m_snapshot ? m_snapshot->revision : 0; }
    bool    reachesEnd() const { return m_end; }
    bool    valid() const {
        return m_snapshot && m_first >= 0 && m_count >= 0 &&
               qint64(m_first) + m_count <= m_snapshot->count() &&
               (! m_tracked || ! m_lifetime.expired());
    }

private:
    friend class LazyView;
    ItemSnapshotPtr           m_snapshot;
    int                       m_first = 0;
    int                       m_count = 0;
    QVector<int>              m_indices;
    bool                      m_end     = false;
    bool                      m_tracked = false;
    std::weak_ptr<const bool> m_lifetime;
};
} // namespace qml_material
Q_DECLARE_METATYPE(qml_material::ItemDemand)
