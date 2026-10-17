// Geometry adapted from Android Open Source Project, Copyright (C) 2022.
// SPDX-License-Identifier: Apache-2.0
#include "qml_material/indicator/squiggly_progress_geometry.hpp"
#include "qml_material/shape/rounded_polygon.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace qml_material
{
namespace
{
qreal parameterAtX(const Cubic& curve, qreal x) {
    if (x <= curve.anchor0.x()) return 0;
    if (x >= curve.anchor1.x()) return 1;
    qreal lo = 0, hi = 1;
    for (int i = 0; i < 32; ++i) {
        const auto mid = (lo + hi) / 2;
        if (curve.point(mid).x() < x)
            lo = mid;
        else
            hi = mid;
    }
    return (lo + hi) / 2;
}
void append(QPainterPath& path, const Cubic& curve, qreal left, qreal right) {
    if (right <= left || curve.anchor1.x() <= left || curve.anchor0.x() >= right) return;
    const auto a    = parameterAtX(curve, left);
    const auto b    = parameterAtX(curve, right);
    auto       part = b < 1 ? curve.split(b).first : curve;
    if (a > 0) part = part.split(a / b).second;
    if (path.isEmpty()) path.moveTo(part.anchor0);
    path.cubicTo(part.control0, part.control1, part.anchor1);
}
} // namespace
SquigglyProgressPaths SquigglyProgressGeometry::render(const SquigglyProgressState& s,
                                                       QSizeF                       size) {
    SquigglyProgressPaths result;
    for (auto value : { s.progress,
                        s.phase,
                        s.amplitudeFraction,
                        s.waveLength,
                        s.waveAmplitude,
                        size.width(),
                        size.height() }) {
        if (! std::isfinite(value)) return result;
    }
    if (size.width() <= 0 || size.height() <= 0 || s.waveLength <= 0 || s.waveAmplitude < 0)
        return result;
    const auto width    = size.width();
    const auto progress = std::clamp(s.progress, 0.0, 1.0);
    const auto end      = width * progress;
    // Bound work for pathological lengths without changing ordinary SystemUI dimensions.
    const auto wavelength    = std::max(s.waveLength, width / 4096);
    const auto phase         = std::fmod(std::fmod(s.phase, wavelength) + wavelength, wavelength);
    const auto start         = -phase - wavelength / 2;
    const auto waveEnd       = s.transitionEnabled ? width : end;
    const auto transitionEnd = width * (progress > .6 ? progress : .2 + progress * 2 / 3);
    const auto amplitude     = s.waveAmplitude * std::clamp(s.amplitudeFraction, 0.0, 1.0);
    const auto sample        = [&](qreal x, qreal sign) {
        const auto coefficient =
            s.transitionEnabled
                ? std::clamp((transitionEnd + .75 * wavelength - x) / (1.5 * wavelength), 0.0, 1.0)
                : 1.0;
        return sign * amplitude * coefficient;
    };
    const auto center = size.height() / 2;
    const auto half   = wavelength / 2;
    qreal      sign   = 1;
    auto       y      = sample(start, sign);
    for (auto x = start; x < waveEnd; x += half) {
        sign              = -sign;
        const auto  nextY = sample(x + half, sign);
        const Cubic curve { { x, center + y },
                            { x + half / 2, center + y },
                            { x + half / 2, center + nextY },
                            { x + half, center + nextY } };
        append(result.active, curve, 0, end);
        if (s.transitionEnabled) append(result.inactive, curve, end, width);
        y = nextY;
    }
    if (! s.transitionEnabled && end < width) {
        result.inactive.moveTo(end, center);
        result.inactive.lineTo(width, center);
    }
    result.startCap = {
        0, center + std::cos(std::abs(start) / wavelength * 2 * std::numbers::pi) * amplitude
    };
    result.valid = true;
    return result;
}
} // namespace qml_material
