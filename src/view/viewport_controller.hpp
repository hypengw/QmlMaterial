#pragma once

#include "linear_layout.hpp"
#include "item_anchor_p.hpp"
#include "qml_material/model/item_source.hpp"
#include <optional>
#include <QSet>

namespace qml_material
{

class ViewportController {
public:
    struct Result {
        bool   accepted;
        double offset;
    };

    const LinearLayout& layout() const { return m_layout; }
    ItemSnapshotPtr     snapshot() const { return m_snapshot; }
    bool                estimated() const { return m_cache.size() < m_layout.count(); }
    bool                measured(int index) const {
        if (! m_snapshot || index < 0 || index >= m_snapshot->count()) return false;
        const auto& row   = m_snapshot->descriptorAt(index);
        const auto  found = m_cache.constFind(row.key);
        return found != m_cache.cend() && found->type == row.reuseType &&
               found->revision == row.contentRevision;
    }

    void clear() {
        m_snapshot.reset();
        m_cache.clear();
        m_layout.reset(0, 0, 0);
    }

    Result apply(ItemSnapshotPtr next, double constraint, double estimate, double spacing,
                 double offset) {
        if (! next || ! std::isfinite(constraint) || constraint < 0 || ! std::isfinite(offset))
            return { false, offset };
        LinearLayout layout;
        if (! layout.reset(next->count(), estimate, spacing)) return { false, offset };

        const auto anchor = ItemAnchor::capture(
            m_snapshot, next, m_layout.indexAt(offset), offset, [this](int index) {
                return m_layout.offset(index);
            });

        QHash<QString, Measurement> cache;
        const bool reuse = m_snapshot && constraint == m_constraint &&
                           ((m_snapshot->stableKeys() && next->stableKeys()) || next == m_snapshot);
        if (reuse) {
            for (int i = 0; i < next->count(); ++i) {
                const auto& row   = next->descriptorAt(i);
                const auto  found = m_cache.constFind(row.key);
                if (found == m_cache.cend() || found->type != row.reuseType ||
                    found->revision != row.contentRevision)
                    continue;
                if (! layout.setExtent(i, found->extent)) return { false, offset };
                cache.insert(row.key, *found);
            }
        }
        double corrected = offset;
        if (next->count() == 0)
            corrected = 0;
        else if (! anchor.key.isEmpty())
            corrected = layout.offset(next->indexOfKey(anchor.key)) - anchor.screenOffset;
        m_snapshot   = std::move(next);
        m_constraint = constraint;
        m_estimate   = estimate;
        m_layout     = std::move(layout);
        m_cache      = std::move(cache);
        return { true, corrected };
    }

    Result applyValues(ItemSnapshotPtr next, const QVector<ItemChange>& changes, double constraint,
                       double estimate, double spacing, double offset) {
        if (! m_snapshot || ! next || next->count() != m_layout.count() ||
            constraint != m_constraint || estimate != m_estimate || spacing != m_layout.spacing())
            return apply(std::move(next), constraint, estimate, spacing, offset);
        if (! std::isfinite(offset)) return { false, offset };
        QVector<int>  dirty;
        QSet<QString> keys;
        for (const auto& change : changes) {
            if (change.kind != ItemChange::Update)
                return apply(std::move(next), constraint, estimate, spacing, offset);
            if (change.index < 0 || change.count <= 0 ||
                qint64(change.index) + change.count > next->count())
                return { false, offset };
            for (int index = change.index; index < change.index + change.count; ++index) {
                const auto& row = next->descriptorAt(index);
                if (m_snapshot->descriptorAt(index).key != row.key)
                    return apply(std::move(next), constraint, estimate, spacing, offset);
                const auto found = m_cache.constFind(row.key);
                if (found == m_cache.cend() ||
                    (found->type == row.reuseType && found->revision == row.contentRevision) ||
                    keys.contains(row.key))
                    continue;
                keys.insert(row.key);
                dirty.append(index);
            }
        }
        // Keep the incremental mutation below the numeric limits checked by setExtent.
        if (! std::isfinite(m_layout.offset(m_layout.count()) + dirty.size() * estimate))
            return apply(std::move(next), constraint, estimate, spacing, offset);
        const int       anchor   = m_layout.indexAt(offset);
        const double    oldStart = m_layout.offset(anchor);
        QVector<double> oldExtents;
        oldExtents.reserve(dirty.size());
        for (int index : dirty) {
            oldExtents.append(m_layout.extent(index));
            if (! m_layout.setExtent(index, estimate)) {
                for (int i = oldExtents.size() - 2; i >= 0; --i)
                    m_layout.setExtent(dirty[i], oldExtents[i]);
                return { false, offset };
            }
        }
        for (const auto& key : keys) m_cache.remove(key);
        m_snapshot = std::move(next);
        return { true, m_layout.count() ? offset + m_layout.offset(anchor) - oldStart : 0 };
    }

    Result measure(int index, double extent, double offset) {
        if (! m_snapshot || index < 0 || index >= m_layout.count() || ! std::isfinite(offset))
            return { false, offset };
        const int    anchor   = m_layout.indexAt(offset);
        const double oldStart = m_layout.offset(anchor);
        if (! m_layout.setExtent(index, extent)) return { false, offset };
        const auto& row = m_snapshot->descriptorAt(index);
        m_cache.insert(row.key, { extent, row.reuseType, row.contentRevision });
        return { true, offset + m_layout.offset(anchor) - oldStart };
    }

    Result invalidate(const QString& key, double offset) {
        if (! m_snapshot) return { false, offset };
        const int index = m_snapshot->indexOfKey(key);
        if (index < 0 || ! std::isfinite(offset)) return { false, offset };
        const int    anchor   = m_layout.indexAt(offset);
        const double oldStart = m_layout.offset(anchor);
        if (! m_layout.setExtent(index, m_estimate)) return { false, offset };
        m_cache.remove(key);
        return { true, offset + m_layout.offset(anchor) - oldStart };
    }

    std::optional<double> positionAt(const QString& key, double viewport, double alignment,
                                     double extraOffset = 0) const {
        if (! m_snapshot || ! std::isfinite(viewport) || viewport < 0 ||
            ! std::isfinite(alignment) || alignment < 0 || alignment > 1 ||
            ! std::isfinite(extraOffset))
            return {};
        const int index = m_snapshot->indexOfKey(key);
        if (index < 0) return {};
        return m_layout.offset(index) - (viewport - m_layout.extent(index)) * alignment +
               extraOffset;
    }

private:
    struct Measurement {
        double  extent;
        QString type;
        quint64 revision;
    };
    ItemSnapshotPtr             m_snapshot;
    LinearLayout                m_layout;
    QHash<QString, Measurement> m_cache;
    double                      m_constraint { 0 };
    double                      m_estimate { 0 };
};

} // namespace qml_material
