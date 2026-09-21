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
} // namespace qml_material::split_layout
