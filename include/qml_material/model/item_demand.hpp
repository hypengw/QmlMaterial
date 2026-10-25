#pragma once

#include <QtQml/qqmlregistration.h>
#include "qml_material/model/item_snapshot.hpp"

namespace qml_material
{
class QML_MATERIAL_API ItemDemand {
    Q_GADGET
    QML_VALUE_TYPE(itemDemand)
    Q_PROPERTY(bool valid READ valid)
    Q_PROPERTY(int first READ first CONSTANT)
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(quint64 revision READ revision CONSTANT)
    Q_PROPERTY(bool reachesEnd READ reachesEnd CONSTANT)
public:
    ItemDemand() = default;
    ItemDemand(ItemSnapshotPtr snapshot, int first, int count, bool reachesEnd = false)
        : m_snapshot(std::move(snapshot)), m_first(first), m_count(count), m_end(reachesEnd) {}
    ItemSnapshotPtr snapshot() const { return m_snapshot; }
    int             first() const { return m_first; }
    int             count() const { return m_count; }
    quint64         revision() const { return m_snapshot ? m_snapshot->revision : 0; }
    bool            reachesEnd() const { return m_end; }
    bool            valid() const {
        return m_snapshot && m_first >= 0 && m_count >= 0 &&
               qint64(m_first) + m_count <= m_snapshot->count() &&
               (! m_tracked || ! m_lifetime.expired());
    }

private:
    friend class LazyList;
    ItemSnapshotPtr           m_snapshot;
    int                       m_first   = 0;
    int                       m_count   = 0;
    bool                      m_end     = false;
    bool                      m_tracked = false;
    std::weak_ptr<const bool> m_lifetime;
};
} // namespace qml_material
Q_DECLARE_METATYPE(qml_material::ItemDemand)
