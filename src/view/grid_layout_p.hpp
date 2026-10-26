#pragma once

#include <QRectF>
#include <Qt>
#include <algorithm>
#include <cmath>
#include <limits>

namespace qml_material
{
struct ItemRange {
    int first = -1;
    int last  = -1;
};

class GridLayout {
public:
    bool reset(int count, qreal crossSize, int fixedSlotCount, qreal minimum, qreal extent,
               qreal mainSpacing, qreal crossSpacing, bool mirrored, bool reversed = false) {
        if (count < 0 || fixedSlotCount < 0 || ! std::isfinite(crossSize) || crossSize < 0 ||
            ! std::isfinite(minimum) || minimum <= 0 || ! std::isfinite(extent) || extent <= 0 ||
            ! std::isfinite(mainSpacing) || mainSpacing < 0 || ! std::isfinite(crossSpacing) ||
            crossSpacing < 0)
            return false;
        const qreal adaptive = std::floor((crossSize + crossSpacing) / (minimum + crossSpacing));
        const int   resolved =
            fixedSlotCount > 0
                ? fixedSlotCount
                : int(std::clamp(adaptive, qreal(1), qreal(std::numeric_limits<int>::max())));
        // Fixed slot counts still fit narrow containers by reducing the effective gap.
        const qreal  gap   = resolved > 1 ? std::min(crossSpacing, crossSize / resolved) : 0;
        const qreal  cell  = (crossSize - (resolved - 1) * gap) / resolved;
        const qint64 lines = (qint64(count) + resolved - 1) / resolved;
        const qreal  total = lines ? lines * extent + (lines - 1) * mainSpacing : 0;
        if (! std::isfinite(total) || ! std::isfinite(cell)) return false;
        m_count        = count;
        m_cross        = crossSize;
        m_slots        = resolved;
        m_cell         = cell;
        m_extent       = extent;
        m_mainSpacing  = mainSpacing;
        m_crossSpacing = gap;
        m_total        = total;
        m_mirrored     = mirrored;
        m_reversed     = reversed;
        return true;
    }
    int   count() const { return m_count; }
    int   fixedSlotCount() const { return m_slots; }
    qreal cellExtent() const { return m_cell; }
    qreal crossSize() const { return m_cross; }
    qreal totalExtent() const { return m_total; }
    qreal offset(int index) const {
        const auto start = rawOffset(index);
        return m_reversed ? m_total - start - m_extent : start;
    }
    bool   reversed() const { return m_reversed; }
    qreal  extent(int) const { return m_extent; }
    QRectF rect(int index) const {
        const int   slot  = index % m_slots;
        const qreal start = slot * (m_cell + m_crossSpacing);
        return { m_mirrored ? m_cross - start - m_cell : start, offset(index), m_cell, m_extent };
    }
    int indexAt(qreal position) const {
        if (! m_count || ! std::isfinite(position)) return -1;
        if (m_reversed) position = m_total - position;
        return rawIndexAt(position);
    }
    int rawIndexAt(qreal position) const {
        if (! m_count || ! std::isfinite(position)) return -1;
        const qreal line = std::clamp(std::floor(position / (m_extent + m_mainSpacing)),
                                      qreal(0),
                                      qreal((m_count - 1) / m_slots));
        return int(line) * m_slots;
    }
    ItemRange visibleRange(qreal begin, qreal end) const {
        if (! m_count || m_cross <= 0 || m_cell <= 0 || ! std::isfinite(begin) ||
            ! std::isfinite(end) || begin >= end || end <= 0 || begin >= m_total)
            return {};
        if (m_reversed) {
            const auto oldBegin = begin;
            begin               = m_total - end;
            end                 = m_total - oldBegin;
        }
        int first = rawIndexAt(begin);
        if (rawOffset(first) + m_extent <= begin) {
            const qint64 next = qint64(first) + m_slots;
            if (next >= m_count) return {};
            first = int(next);
        }
        int last = rawIndexAt(std::min(end, m_total));
        if (rawOffset(last) >= end) last -= m_slots;
        if (last < first) return {};
        last = int(std::min(qint64(m_count - 1), qint64(last) + m_slots - 1));
        return { first, last };
    }
    ItemRange requestedRange(qreal offset, qreal viewport, qreal cache) const {
        const auto desired = visibleRange(offset - cache, offset + viewport + cache);
        const auto visible = visibleRange(offset, offset + viewport);
        if (desired.first < 0) return {};
        // Cache whole lines near the viewport; visible cells are never budget-limited.
        const int lines       = std::max(1, 128 / m_slots);
        const int centerFirst = visible.first >= 0 ? visible.first : indexAt(offset);
        const int centerLast  = visible.last >= 0 ? visible.last : centerFirst;
        return { int(std::max(qint64(desired.first),
                              qint64(centerFirst) - qint64(lines) * m_slots)),
                 int(std::min(qint64(desired.last),
                              (qint64(centerLast / m_slots) + lines + 1) * m_slots - 1)) };
    }
    int neighbor(int index, int lineDelta, int slotDelta) const {
        if (index < 0 || index >= m_count) return m_count ? 0 : -1;
        if (m_mirrored) slotDelta = -slotDelta;
        if (m_reversed) lineDelta = -lineDelta;
        const int    slot = index % m_slots;
        const qint64 line = qint64(index / m_slots) + lineDelta;
        if (line < 0 || line > (m_count - 1) / m_slots || slot + slotDelta < 0 ||
            slot + slotDelta >= m_slots)
            return index;
        return int(std::min(qint64(m_count - 1), line * m_slots + slot + slotDelta));
    }

private:
    qreal rawOffset(int index) const { return (index / m_slots) * (m_extent + m_mainSpacing); }
    int   m_count = 0;
    int   m_slots = 1;
    qreal m_cross = 0, m_cell = 0, m_extent = 48, m_mainSpacing = 0, m_crossSpacing = 0,
          m_total    = 0;
    bool  m_mirrored = false;
    bool  m_reversed = false;
};
} // namespace qml_material
