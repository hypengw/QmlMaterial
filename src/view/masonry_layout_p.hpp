#pragma once

#include "lazy_view_layout_p.hpp"
#include "ordered_extent_index_p.hpp"
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>
#include <limits>
#include <set>

namespace qml_material
{
class MasonryViewLayout final : public LazyViewLayout {
public:
    int   fixedSlotCount = 0;
    qreal minimum = 140, estimate = 140, mainSpacing = 0, crossSpacing = 0;
    bool  mirrored = false;

    int   slotCount() const { return m_columns.fixedSlotCount(); }
    qreal cellExtent() const { return m_columns.cellExtent(); }
    bool supportsOrientation(Qt::Orientation value) const override { return value == Qt::Vertical; }
    bool discreteItems() const override { return true; }
    ItemSnapshotPtr snapshot() const override { return m_snapshot; }
    int             count() const override { return m_locations.size(); }
    bool            estimated() const override { return m_cache.size() < count(); }
    void            clear() override {
        m_snapshot.reset();
        m_locations.clear();
        m_lanes.clear();
        m_cache.clear();
        m_bound.clear();
        m_columns = {};
    }
    Result apply(ItemSnapshotPtr next, qreal cross, qreal position, qreal) override {
        if (! next || ! std::isfinite(position)) return { false, position };
        GridLayout columns;
        if (! columns.reset(next->count(),
                            cross,
                            fixedSlotCount,
                            minimum,
                            estimate,
                            mainSpacing,
                            crossSpacing,
                            mirrored))
            return { false, position };
        const auto anchor =
            ItemAnchor::capture(m_snapshot, next, indexAt(position), position, [this](int index) {
                return offset(index);
            });
        const bool sameColumns = m_snapshot &&
                                 m_columns.fixedSlotCount() == columns.fixedSlotCount() &&
                                 m_columns.cellExtent() == columns.cellExtent();
        const bool stable =
            m_snapshot && ((m_snapshot->stableKeys() && next->stableKeys()) || m_snapshot == next);
        int prefix = 0;
        if (sameColumns && stable) {
            while (prefix < std::min(count(), next->count()) &&
                   m_snapshot->descriptorAt(prefix).key == next->descriptorAt(prefix).key)
                ++prefix;
        }
        const int laneCount = std::min(columns.fixedSlotCount(), std::max(1, next->count()));
        std::vector<OrderedExtentIndex> lanes;
        lanes.reserve(laneCount);
        for (int i = 0; i < laneCount; ++i) lanes.emplace_back(mainSpacing);
        QVector<int> locations;
        locations.reserve(next->count());
        QHash<QString, Measurement>     cache;
        QSet<QString>                   bound;
        std::set<std::pair<qreal, int>> frontier;
        for (int lane = 0; lane < laneCount; ++lane) frontier.emplace(0, lane);
        for (int i = 0; i < next->count(); ++i) {
            const auto& row      = next->descriptorAt(i);
            qreal       height   = estimate;
            const auto  measured = m_cache.constFind(row.key);
            if (sameColumns && stable && measured != m_cache.cend() &&
                measured->type == row.reuseType && measured->revision == row.contentRevision) {
                height = measured->extent;
                cache.insert(row.key, *measured);
            }
            const bool keepLane = i < prefix && m_bound.contains(row.key);
            const int  lane     = keepLane ? m_locations[i] : frontier.begin()->second;
            frontier.erase({ lanes[lane].prefix(i), lane });
            if (! lanes[lane].set(i, height)) return { false, position };
            locations.append(lane);
            if (keepLane) bound.insert(row.key);
            frontier.emplace(lanes[lane].prefix(i + 1), lane);
        }
        m_snapshot      = std::move(next);
        m_columns       = columns;
        m_lanes         = std::move(lanes);
        m_locations     = std::move(locations);
        m_cache         = std::move(cache);
        m_bound         = std::move(bound);
        qreal corrected = count() ? position : 0;
        if (! anchor.key.isEmpty())
            corrected = offset(m_snapshot->indexOfKey(anchor.key)) - anchor.screenOffset;
        return { true, corrected };
    }
    Result measure(int index, qreal value, qreal position) override {
        if (index < 0 || index >= count() || ! std::isfinite(position) || ! std::isfinite(value) ||
            value < 0)
            return { false, position };
        const int   anchor  = indexAt(position);
        const qreal before  = offset(anchor);
        const auto& row     = m_snapshot->descriptorAt(index);
        const int   oldLane = m_locations[index];
        int         lane    = oldLane;
        if (! m_bound.contains(row.key)) {
            // Estimates do not commit a lane until the item has actually been measured.
            qreal shortest = std::numeric_limits<qreal>::infinity();
            for (int candidate = 0; candidate < int(m_lanes.size()); ++candidate) {
                const qreal end = m_lanes[candidate].prefix(index);
                if (end < shortest) {
                    shortest = end;
                    lane     = candidate;
                }
            }
        }
        if (! m_lanes[lane].set(index, value)) return { false, position };
        if (lane != oldLane) {
            m_lanes[oldLane].erase(index);
            m_locations[index] = lane;
        }
        m_cache.insert(row.key, { value, row.reuseType, row.contentRevision });
        m_bound.insert(row.key);
        return { true, position + offset(anchor) - before };
    }
    qreal offset(int index) const override {
        return index >= 0 && index < count() ? m_lanes[m_locations[index]].prefix(index) : 0;
    }
    qreal extent(int index) const override {
        return index >= 0 && index < count() ? m_lanes[m_locations[index]].extent(index) : 0;
    }
    qreal totalExtent() const override {
        qreal result = 0;
        for (const auto& lane : m_lanes) result = std::max(result, lane.totalExtent());
        return result;
    }
    QRectF rect(int index) const override {
        if (index < 0 || index >= count()) return {};
        const auto cell = m_columns.rect(m_locations[index]);
        return { cell.x(), offset(index), cell.width(), extent(index) };
    }
    int indexAt(qreal position) const override {
        int   result     = -1;
        bool  intersects = false;
        qreal distance   = std::numeric_limits<qreal>::infinity();
        for (const auto& lane : m_lanes) {
            const int candidate = lane.indexAt(position);
            if (candidate < 0) continue;
            const bool contains =
                offset(candidate) <= position && offset(candidate) + extent(candidate) > position;
            const qreal delta = std::abs(offset(candidate) - position);
            if ((contains && ! intersects) ||
                (contains == intersects &&
                 (delta < distance || (delta == distance && candidate < result)))) {
                result     = candidate;
                distance   = delta;
                intersects = contains;
            }
        }
        return result;
    }
    QVector<int> indices(qreal begin, qreal end) const {
        QVector<int> result;
        if (cellExtent() <= 0) return result;
        for (const auto& lane : m_lanes)
            for (int index : lane.intersecting(begin, end)) result.append(index);
        std::sort(result.begin(), result.end());
        return result;
    }
    ItemRange visibleRange(qreal begin, qreal end) const override {
        const auto items = indices(begin, end);
        return items.isEmpty() ? ItemRange {} : ItemRange { items.first(), items.last() };
    }
    QVector<int> requestedIndices(qreal position, qreal viewport, qreal cache) const override {
        auto         requested = indices(position, position + viewport);
        QSet<int>    selected(requested.begin(), requested.end());
        QVector<int> candidates;
        for (const auto& lane : m_lanes) {
            for (int index : lane.intersecting(position - cache, position, 128, true)) {
                if (! selected.contains(index)) {
                    candidates.append(index);
                    selected.insert(index);
                }
            }
            for (int index :
                 lane.intersecting(position + viewport, position + viewport + cache, 128)) {
                if (! selected.contains(index)) {
                    candidates.append(index);
                    selected.insert(index);
                }
            }
        }
        const auto distance = [&](int index) {
            return std::max({ position - offset(index) - extent(index),
                              offset(index) - position - viewport,
                              qreal(0) });
        };
        std::sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            const qreal da = distance(a), db = distance(b);
            return da < db || (da == db && a < b);
        });
        if (candidates.size() > 256) candidates.resize(256);
        requested.append(candidates);
        std::sort(requested.begin(), requested.end());
        return requested;
    }
    ItemRange requestedRange(qreal position, qreal viewport, qreal cache) const override {
        const auto items = requestedIndices(position, viewport, cache);
        return items.isEmpty() ? ItemRange {} : ItemRange { items.first(), items.last() };
    }
    ItemRange demandRange(qreal position, qreal viewport, qreal cache) const override {
        return requestedRange(position, viewport, cache);
    }
    bool reachesEnd(qreal position, qreal viewport, qreal cache) const override {
        if (! count()) return true;
        for (const auto& lane : m_lanes)
            if (lane.totalExtent() <= position + viewport + cache) return true;
        return false;
    }
    int neighbor(int index, int lineDelta, int slotDelta) const override {
        if (index < 0 || index >= count()) return count() ? 0 : -1;
        const int sourceLane = m_locations[index];
        if (lineDelta) {
            int result = index;
            for (qint64 i = 0; i < std::abs(qint64(lineDelta)); ++i) {
                const int next = lineDelta > 0 ? m_lanes[sourceLane].next(result)
                                               : m_lanes[sourceLane].previous(result);
                if (next < 0) break;
                result = next;
            }
            return result;
        }
        const int lane = sourceLane + (mirrored ? -slotDelta : slotDelta);
        if (lane < 0 || lane >= int(m_lanes.size())) return index;
        const int target = m_lanes[lane].nearestCenter(offset(index) + extent(index) / 2);
        return target >= 0 ? target : index;
    }

private:
    struct Measurement {
        qreal   extent;
        QString type;
        quint64 revision;
    };
    ItemSnapshotPtr                 m_snapshot;
    GridLayout                      m_columns;
    std::vector<OrderedExtentIndex> m_lanes;
    QVector<int>                    m_locations;
    QHash<QString, Measurement>     m_cache;
    QSet<QString>                   m_bound;
};
} // namespace qml_material
