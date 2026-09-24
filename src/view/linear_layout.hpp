#pragma once

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

namespace qml_material
{

class LinearLayout {
public:
    bool reset(int count, double estimate, double spacing) {
        if (count < 0 || ! validExtent(estimate) || ! validExtent(spacing) ||
            ! std::isfinite((estimate + spacing) * count))
            return false;
        m_spacing = spacing;
        m_extents.assign(count, estimate);
        m_positive.clear();
        if (estimate > 0)
            for (int i = 0; i < count; ++i) m_positive.insert(m_positive.end(), i);
        m_tree.assign(size_t(count) + 1, 0);
        for (size_t i = 1; i < m_tree.size(); ++i) {
            m_tree[i] += estimate + spacing;
            const size_t parent = i + lowBit(i);
            if (parent < m_tree.size()) m_tree[parent] += m_tree[i];
        }
        return true;
    }

    int    count() const { return int(m_extents.size()); }
    double extent(int index) const { return m_extents.at(index); }
    double spacing() const { return m_spacing; }

    bool setExtent(int index, double value) {
        if (index < 0 || index >= count() || ! validExtent(value)) return false;
        if (! std::isfinite(offset(count()) - m_extents[index] + value)) return false;
        const double delta = value - m_extents[index];
        m_extents[index]   = value;
        if (value > 0)
            m_positive.insert(index);
        else
            m_positive.erase(index);
        for (size_t i = size_t(index) + 1; i < m_tree.size(); i += lowBit(i)) m_tree[i] += delta;
        return true;
    }

    double offset(int index) const {
        double result = 0;
        for (size_t i = size_t(std::clamp(index, 0, count())); i; i -= lowBit(i))
            result += m_tree[i];
        return result;
    }

    double totalExtent() const { return count() ? std::max(0.0, offset(count()) - m_spacing) : 0; }

    // Gaps belong to the preceding row; equal starts select the last zero-sized row.
    int indexAt(double position) const {
        if (! count() || ! std::isfinite(position)) return -1;
        if (position < 0) return 0;
        size_t index = 0;
        size_t bit   = 1;
        while (bit <= size_t(count()) / 2) bit <<= 1;
        double prefix = 0;
        for (; bit; bit >>= 1) {
            const size_t next = index + bit;
            if (next < m_tree.size() && prefix + m_tree[next] <= position) {
                prefix += m_tree[next];
                index = next;
            }
        }
        return std::min(int(index), count() - 1);
    }

    struct Range {
        int first = -1;
        int last  = -1;
    };

    // Positive-area intersections with the half-open viewport, excluding spacing.
    Range visibleRange(double begin, double end) const {
        if (! std::isfinite(begin) || ! std::isfinite(end) || begin >= end ||
            m_positive.empty())
            return {};
        int first = indexAt(begin);
        if (offset(first) + extent(first) <= begin) ++first;
        auto it = m_positive.lower_bound(first);
        if (it == m_positive.end() || offset(*it) >= end) return {};
        first = *it;
        int last = indexAt(end);
        if (offset(last) >= end) --last;
        auto past = m_positive.upper_bound(last);
        if (past == m_positive.begin()) return {};
        --past;
        return { first, *past };
    }

private:
    static size_t       lowBit(size_t value) { return value & (~value + 1); }
    static bool         validExtent(double value) { return std::isfinite(value) && value >= 0; }
    double              m_spacing { 0 };
    std::vector<double> m_extents;
    std::vector<double> m_tree;
    std::set<int>       m_positive;
};

} // namespace qml_material
