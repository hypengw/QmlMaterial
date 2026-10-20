#pragma once
#include <Qt>
#include <algorithm>
#include <cmath>
#include <optional>

namespace qml_material
{
inline std::optional<qreal> sliderKeyPosition(int key, qreal position, qreal step,
                                              Qt::Orientation orientation, bool reversed) {
    const qreal delta     = step > 0 ? step : .01;
    const qreal sign      = reversed ? -1 : 1;
    const qreal page      = std::clamp(std::floor(1 / delta / 10), qreal(1), qreal(10));
    qreal       direction = 0;
    switch (key) {
    case Qt::Key_Home: return 0;
    case Qt::Key_End: return 1;
    case Qt::Key_Right:
        if (orientation != Qt::Horizontal) return {};
        direction = sign;
        break;
    case Qt::Key_Left:
        if (orientation != Qt::Horizontal) return {};
        direction = -sign;
        break;
    case Qt::Key_Up:
        if (orientation != Qt::Vertical) return {};
        direction = -sign;
        break;
    case Qt::Key_Down:
        if (orientation != Qt::Vertical) return {};
        direction = sign;
        break;
    case Qt::Key_PageUp: direction = page * (orientation == Qt::Vertical ? -sign : 1); break;
    case Qt::Key_PageDown: direction = page * (orientation == Qt::Vertical ? sign : -1); break;
    default: return {};
    }
    return std::clamp(position + direction * delta, qreal(0), qreal(1));
}
} // namespace qml_material
