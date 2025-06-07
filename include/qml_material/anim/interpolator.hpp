#pragma once

#include <QtCore/QEasingCurve>
#include <QtCore/QPointF>

namespace qml_material::anim
{
inline auto fast_out_slow_in() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4f, 0.f }, { 0.2f, 1.0f }, { 1.0f, 1.0f });
    return out;
}
} // namespace qml_material::anim