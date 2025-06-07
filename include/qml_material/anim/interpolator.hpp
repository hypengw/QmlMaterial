#pragma once

#include <QtCore/QEasingCurve>
#include <QtCore/QPointF>

namespace qml_material::anim
{
inline auto emphasized_accelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.3, 0. }, { 0.8, 0.15 }, { 1., 1. });
    return out;
}

inline auto emphasized_decelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.05, 0.7 }, { 0.1, 1. }, { 1., 1. });
    return out;
}

inline auto standard() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.2, 0. }, { 0., 1. }, { 1., 1. });
    return out;
}

inline auto standard_accelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.3, 0. }, { 1., 1. }, { 1., 1. });
    return out;
}

inline auto standard_decelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0., 0. }, { 0., 1. }, { 1., 1. });
    return out;
}

inline auto legacy() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0. }, { 0.2, 1. }, { 1., 1. });
    return out;
}

inline auto legacy_accelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0. }, { 1., 1. }, { 1., 1. });
    return out;
}

inline auto legacy_decelerate() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0., 0. }, { 0.2, 1. }, { 1., 1. });
    return out;
}

inline auto linear() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0., 0. }, { 1., 1. }, { 1., 1. });
    return out;
}

inline auto fast_out_slow_in() noexcept -> QEasingCurve {
    auto out = QEasingCurve(QEasingCurve::BezierSpline);
    out.addCubicBezierSegment({ 0.4, 0. }, { 0.2, 1. }, { 1., 1. });
    return out;
}
} // namespace qml_material::anim