#pragma once

#include "linear_layout.hpp"
#include "qml_material/model/item_source.hpp"
#include <optional>

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

    void clear() {
        m_snapshot.reset();
        m_cache.clear();
        m_layout.reset(0, 0, 0);
    }

    Result apply(ItemSnapshotPtr next, double width, double estimate, double spacing,
                 double offset) {
        if (! next || ! std::isfinite(width) || width < 0 || ! std::isfinite(offset))
            return { false, offset };
        LinearLayout layout;
        if (! layout.reset(int(next->rows.size()), estimate, spacing)) return { false, offset };

        QString anchor;
        double  screenOffset = 0;
        if (m_snapshot && m_snapshot->stableKeys && next->stableKeys) {
            const int oldIndex = m_layout.indexAt(offset);
            auto      select   = [&](int index) {
                const auto& key = m_snapshot->rows[index].key;
                if (next->indexOfKey(key) < 0) return false;
                anchor       = key;
                screenOffset = m_layout.offset(index) - offset;
                return true;
            };
            if (oldIndex >= 0 && ! select(oldIndex)) {
                for (int i = oldIndex + 1; i < m_layout.count() && anchor.isEmpty(); ++i) select(i);
                for (int i = oldIndex - 1; i >= 0 && anchor.isEmpty(); --i) select(i);
            }
        }

        QHash<QString, Measurement> cache;
        const bool reuse = m_snapshot && width == m_width &&
                           ((m_snapshot->stableKeys && next->stableKeys) || next == m_snapshot);
        if (reuse) {
            for (int i = 0; i < next->rows.size(); ++i) {
                const auto& row   = next->rows[i];
                const auto  found = m_cache.constFind(row.key);
                if (found == m_cache.cend() || found->value != row.value ||
                    found->type != row.reuseType || found->revision != row.contentRevision)
                    continue;
                if (! layout.setExtent(i, found->height)) return { false, offset };
                cache.insert(row.key, *found);
            }
        }
        double corrected = offset;
        if (next->rows.isEmpty())
            corrected = 0;
        else if (! anchor.isEmpty())
            corrected = layout.offset(next->indexOfKey(anchor)) - screenOffset;
        m_snapshot = std::move(next);
        m_width    = width;
        m_estimate = estimate;
        m_layout   = std::move(layout);
        m_cache    = std::move(cache);
        return { true, corrected };
    }

    Result measure(int index, double height, double offset) {
        if (! m_snapshot || index < 0 || index >= m_layout.count() || ! std::isfinite(offset))
            return { false, offset };
        const int    anchor   = m_layout.indexAt(offset);
        const double oldStart = m_layout.offset(anchor);
        if (! m_layout.setExtent(index, height)) return { false, offset };
        const auto& row = m_snapshot->rows[index];
        m_cache.insert(row.key, { height, row.value, row.reuseType, row.contentRevision });
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
        double   height;
        QVariant value;
        QString  type;
        quint64  revision;
    };
    ItemSnapshotPtr             m_snapshot;
    LinearLayout                m_layout;
    QHash<QString, Measurement> m_cache;
    double                      m_width { 0 };
    double                      m_estimate { 0 };
};

} // namespace qml_material
