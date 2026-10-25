#pragma once

#include "qml_material/input/nested_scroll.hpp"
#include <algorithm>
#include <cmath>

namespace qml_material
{
inline Qt::Orientations axesFor(NestedScroll* config) {
    return config->enabled() ? config->axes() : Qt::Horizontal | Qt::Vertical;
}
inline NestedScrollConnection* connectionFor(NestedScroll* config) {
    return config->enabled() ? config->connection() : nullptr;
}
inline bool wheelEnabledFor(NestedScroll* config) {
    return ! config->enabled() || config->wheelEnabled();
}
inline QPointF vectorToScene(QQuickItem* item, QPointF delta) {
    return item->mapToScene(delta) - item->mapToScene(QPointF());
}
inline QPointF vectorFromScene(QQuickItem* item, QPointF delta) {
    return item->mapFromScene(delta) - item->mapFromScene(QPointF());
}
inline bool    finite(QPointF p) { return std::isfinite(p.x()) && std::isfinite(p.y()); }
inline QPointF boundedConsumption(QPointF requested, QPointF available) {
    const auto bounded = [](qreal value, qreal limit) {
        return std::isfinite(value)
                   ? std::clamp(value, std::min(qreal(0), limit), std::max(qreal(0), limit))
                   : 0;
    };
    return { bounded(requested.x(), available.x()), bounded(requested.y(), available.y()) };
}

} // namespace qml_material
