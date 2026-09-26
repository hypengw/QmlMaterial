#include "split_layout_p.hpp"

#include <algorithm>
#include <cmath>

namespace qml_material::split_layout
{
namespace
{
qreal finiteSize(qreal value) { return std::isfinite(value) ? std::max(qreal(0), value) : 0; }
} // namespace

Resize resize(const QList<Pane>& panes, qreal available, int handleIndex, qreal delta) {
    const auto initial = calculate(panes, available);
    if (handleIndex < 0 || handleIndex >= panes.size() || ! std::isfinite(delta) ||
        ! initial.panes[handleIndex].handleVisible)
        return {};
    int        index   = handleIndex;
    const bool leading = initial.fillIndex > handleIndex;
    if (! leading) {
        do {
            ++index;
        } while (index < panes.size() && ! panes[index].visible);
    }
    if (index >= panes.size()) return {};
    const qreal minimum      = finiteSize(panes[index].minimum);
    const qreal capacity     = initial.panes[index].size + initial.panes[initial.fillIndex].size +
                               std::max(qreal(0), finiteSize(available) - initial.extent) -
                               finiteSize(panes[initial.fillIndex].minimum);
    const qreal requested    = std::clamp(initial.panes[index].size + (leading ? delta : -delta),
                                          minimum,
                                          std::max(minimum, capacity));
    auto        updated      = panes;
    updated[index].preferred = requested;
    return { index, calculate(updated, available).panes[index].size };
}

Result calculate(const QList<Pane>& panes, qreal available) {
    Result result;
    result.panes.resize(panes.size());
    QList<qreal> minimums(panes.size());
    QList<qreal> maximums(panes.size());
    int          lastVisible = -1;
    for (int i = 0; i < panes.size(); ++i) {
        const auto& pane = panes[i];
        if (! pane.visible) continue;
        lastVisible = i;
        if (pane.fill && result.fillIndex == -1) result.fillIndex = i;
        minimums[i] = finiteSize(pane.minimum);
        // Contradictory bounds preserve the minimum instead of reaching qBound with min > max.
        maximums[i] = std::isnan(pane.maximum) ? std::numeric_limits<qreal>::infinity()
                                               : std::max(minimums[i], pane.maximum);
    }
    if (result.fillIndex == -1) result.fillIndex = lastVisible;
    if (lastVisible == -1) return result;

    qreal used = 0;
    for (int i = 0; i < panes.size(); ++i) {
        const auto& pane     = panes[i];
        auto&       geometry = result.panes[i];
        if (! pane.visible) continue;
        geometry.visible       = true;
        geometry.handleVisible = i != lastVisible;
        geometry.handleSize    = geometry.handleVisible ? finiteSize(pane.handleSize) : 0;
        used += geometry.handleSize;
        const qreal preferred  = pane.preferred && std::isfinite(*pane.preferred)
                                     ? *pane.preferred
                                     : finiteSize(pane.implicitSize);
        geometry.preferredSize = std::clamp(preferred, minimums[i], maximums[i]);
        result.implicitExtent += geometry.preferredSize + geometry.handleSize;
        if (i == result.fillIndex) continue;
        geometry.size = geometry.preferredSize;
        used += geometry.size;
    }

    auto& fill = result.panes[result.fillIndex];
    available  = finiteSize(available);
    fill.size =
        std::clamp(available - used, minimums[result.fillIndex], maximums[result.fillIndex]);
    used += fill.size;
    qreal excess = std::max(qreal(0), used - available);
    for (int i = panes.size() - 1; i >= 0 && excess > 0; --i) {
        if (! panes[i].visible || i == result.fillIndex) continue;
        auto&       geometry  = result.panes[i];
        const qreal reduction = std::min(excess, geometry.size - minimums[i]);
        geometry.size -= reduction;
        excess -= reduction;
    }

    for (auto& geometry : result.panes) {
        if (! geometry.visible) continue;
        geometry.position       = result.extent;
        geometry.handlePosition = geometry.position + geometry.size;
        result.extent           = geometry.handlePosition + geometry.handleSize;
    }
    return result;
}
Result reveal(const QList<Pane>& panes, const QList<qreal>& progress,
              const QList<qreal>& expandedSizes, qreal available) {
    auto result    = calculate(panes, available);
    available      = finiteSize(available);
    int   receiver = -1;
    qreal used     = 0;
    for (int i = 0; i < panes.size(); ++i) {
        auto& geometry = result.panes[i];
        if (! geometry.visible) continue;
        const qreal amount = finiteSize(progress[i]);
        geometry.size = std::min(expandedSizes[i] * amount, std::max(qreal(0), panes[i].maximum));
        int next      = i + 1;
        while (next < panes.size() && ! panes[next].visible) ++next;
        geometry.handleSize *=
            next < panes.size() ? std::min({ qreal(1), amount, finiteSize(progress[next]) }) : 0;
        geometry.handleVisible = geometry.handleSize > 0;
        used += geometry.size + geometry.handleSize;
        if (progress[i] == 1 && (receiver < 0 || i == result.fillIndex)) receiver = i;
    }
    if (used < available && receiver >= 0) {
        auto& geometry = result.panes[receiver];
        geometry.size +=
            std::min(available - used, std::max(qreal(0), panes[receiver].maximum - geometry.size));
    }
    // Preserve the spring's state; constrain only the geometry shown in the viewport.
    if (used > available) {
        qreal excess = used - available;
        for (int pass = 0; pass < 2 && excess > 0; ++pass) {
            for (int i = panes.size() - 1; i >= 0 && excess > 0; --i) {
                if ((progress[i] == 1) != (pass == 1)) continue;
                auto&       geometry = result.panes[i];
                const qreal minimum  = pass == 0 ? 0 : finiteSize(panes[i].minimum);
                const qreal reduction =
                    std::min(excess, std::max(qreal(0), geometry.size - minimum));
                geometry.size -= reduction;
                excess -= reduction;
            }
        }
        for (int i = panes.size() - 1; i >= 0 && excess > 0; --i) {
            auto&       geometry  = result.panes[i];
            const qreal reduction = std::min(excess, geometry.handleSize);
            geometry.handleSize -= reduction;
            geometry.handleVisible = geometry.handleSize > 0;
            excess -= reduction;
        }
    }
    result.extent = 0;
    for (auto& geometry : result.panes) {
        geometry.position       = result.extent;
        geometry.handlePosition = geometry.position + geometry.size;
        result.extent           = geometry.handlePosition + geometry.handleSize;
    }
    return result;
}
} // namespace qml_material::split_layout
