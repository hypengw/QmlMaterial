// Geometry adapted from Android Open Source Project, Copyright (C) 2020.
// SPDX-License-Identifier: Apache-2.0
#include "qml_material/indicator/progress_indicator_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace qml_material
{
namespace
{
constexpr qreal    tau = 2 * std::numbers::pi;
std::vector<Cubic> circle(qreal radius) {
    constexpr qreal    k = 0.5522847498307936;
    std::vector<Cubic> curves;
    for (int i = 0; i < 8; ++i) {
        const auto a = i * tau / 4;
        const auto b = (i + 1) * tau / 4;
        QPointF    p(std::cos(a) * radius, std::sin(a) * radius);
        QPointF    q(std::cos(b) * radius, std::sin(b) * radius);
        QPointF    t(-std::sin(a) * radius * k, std::cos(a) * radius * k);
        QPointF    u(-std::sin(b) * radius * k, std::cos(b) * radius * k);
        curves.push_back({ p, p + t, q - u, q });
    }
    return curves;
}
qreal ramp(qreal p) { return std::clamp(p / 0.01, 0.0, 1.0); }
qreal unit(qreal p) { return std::isfinite(p) ? std::clamp(p, 0.0, 1.0) : 0; }
} // namespace

QPointF ProgressIndicatorGeometry::lineBounds(qreal start, qreal end, qreal gap, qreal stroke,
                                              qreal width) {
    if (! std::isfinite(start) || ! std::isfinite(end) || ! std::isfinite(gap) ||
        ! std::isfinite(stroke) || ! std::isfinite(width))
        return {};
    start = unit(start);
    end   = unit(end);
    gap += stroke / 2;
    const auto a = width * start + gap * ramp(start);
    const auto b = width * end - gap * ramp(1 - end);
    return a < b ? QPointF(a, b) : QPointF {};
}

void ProgressIndicatorGeometry::rebuild(const ProgressIndicatorState& s, QSizeF size) {
    const auto amplitude =
        s.circular ? s.wavy ? std::max(0.0, s.waveAmplitude * s.amplitudeFraction) : 0.0 : 1.0;
    if (m_size == size && m_circular == s.circular && m_wavy == s.wavy &&
        m_stroke == s.strokeWidth && m_wavelength == s.waveLength && m_amplitude == amplitude &&
        m_legacyRadius == s.legacyRadius && m_legacyCenter == s.legacyCenter)
        return;
    const auto oldCircular = m_circular;
    const auto oldRadius   = m_radius;
    m_size                 = size;
    m_circular             = s.circular;
    m_wavy                 = s.wavy;
    m_legacyRadius         = s.legacyRadius;
    m_legacyCenter         = s.legacyCenter;
    m_stroke               = s.strokeWidth;
    m_wavelength           = s.waveLength;
    m_amplitude            = amplitude;
    m_radius = std::max(0.0,
                        ((s.legacyCenter ? size.height() : std::min(size.width(), size.height())) -
                         (s.legacyRadius ? 0 : s.strokeWidth)) /
                            2);
    if (s.circular && (! oldCircular || oldRadius != m_radius))
        m_circle = MeasuredPath(circle(m_radius));
    else if (! s.circular)
        m_circle = MeasuredPath {};
    const auto length = s.circular ? m_circle.length() / 2 : size.width();
    // Cap pathological configuration costs while preserving ordinary token sizes.
    m_cycles           = s.wavy && s.waveLength > 0
                             ? std::clamp(int(std::min(4096.0, std::floor(length / s.waveLength))),
                                          s.circular ? 3 : 1,
                                          4096)
                             : 0;
    m_actualWavelength = m_cycles ? length / m_cycles : 0;
    std::vector<Cubic> curves;
    if (m_cycles && length > 0) {
        if (! s.circular) {
            const auto half = m_actualWavelength / 2;
            for (int i = 0; i < 2 * (m_cycles + 1); ++i) {
                const auto x = i * half;
                const auto y = i % 2 ? -1.0 : 1.0;
                curves.push_back({ { x, y },
                                   { x + half * 0.48, y },
                                   { x + half * 0.52, -y },
                                   { x + half, -y } });
            }
        } else {
            const auto halfLength = m_circle.length() / (4 * m_cycles);
            for (int i = 0; i < 4 * m_cycles; ++i) {
                auto       a      = m_circle.location(i * halfLength);
                auto       b      = m_circle.location((i + 1) * halfLength);
                const auto inward = [amplitude](QPointF p) {
                    const auto norm = std::hypot(p.x(), p.y());
                    return norm > 0 ? p * std::max(0.0, 1 - 2 * amplitude / norm) : p;
                };
                if (i % 2)
                    a.point = inward(a.point);
                else
                    b.point = inward(b.point);
                const auto control = m_actualWavelength * 0.24;
                curves.push_back({ a.point,
                                   a.point + a.tangent * control,
                                   b.point - b.tangent * control,
                                   b.point });
            }
        }
    }
    m_wave = MeasuredPath(std::move(curves));
    ++m_revision;
}

const std::vector<ProgressDrawPath>&
ProgressIndicatorGeometry::render(const ProgressIndicatorState& s, QSizeF size) {
    auto& result = m_result;
    result.clear();
    if (! std::isfinite(size.width()) || ! std::isfinite(size.height()) || size.width() <= 0 ||
        size.height() <= 0 || ! std::isfinite(s.strokeWidth) || s.strokeWidth <= 0 ||
        ! std::isfinite(s.waveLength) || ! std::isfinite(s.waveAmplitude) ||
        ! std::isfinite(s.phase) || ! std::isfinite(s.amplitudeFraction) || s.waveLength < 0 ||
        s.waveAmplitude < 0 || ! std::isfinite(s.startAngle) || ! std::isfinite(s.rotation) ||
        ! std::isfinite(s.gapSize) || ! std::isfinite(s.gapAngle) ||
        ! std::isfinite(s.trackStart) || ! std::isfinite(s.trackSweep))
        return result;
    rebuild(s, size);
    const auto drain  = unit(s.drain);
    const auto stroke = s.strokeWidth * (s.circular ? 1 : 1 - std::max(0.0, (drain - 0.67) / 0.33));
    const bool wave   = s.wavy && s.waveLength > 0 && s.waveAmplitude > 0 &&
                        s.amplitudeFraction > 0 && m_wave.length() > 0;
    QTransform mirror;
    if (s.mirrored) {
        mirror.translate(size.width(), 0);
        mirror.scale(-1, 1);
    }
    auto append = [&](QPainterPath p, QColor color, qreal width) {
        if (! p.isEmpty()) result.push_back({ mirror.map(p), color, width });
    };
    if (! s.circular) {
        const auto w    = size.width();
        const auto y    = size.height() / 2;
        auto       line = [&](qreal a, qreal b, QColor color) {
            if (b <= a) return;
            if (b - a < stroke) {
                QPainterPath p;
                const auto   diameter = b - a;
                p.addEllipse(QPointF((a + b) / 2, y), diameter / 2, diameter / 2);
                append(p, color, 0);
                return;
            }
            QPainterPath p;
            p.moveTo(a + stroke / 2, y);
            p.lineTo(b - stroke / 2, y);
            append(p, color, stroke);
        };
        auto segments = s.segments;
        std::erase_if(segments, [](const auto& segment) {
            return ! std::isfinite(segment.start) || ! std::isfinite(segment.end);
        });
        std::sort(segments.begin(), segments.end(), [](const auto& a, const auto& b) {
            return a.start < b.start;
        });
        qreal previous    = 0;
        qreal previousGap = 0;
        for (const auto& segment : segments) {
            const auto a        = unit(segment.start);
            const auto b        = unit(segment.end);
            const auto trackGap = s.legacySegments ? segment.gapSize / 2 : s.gapSize;
            const auto gap =
                std::max(0.0, segment.trackGapSize >= 0 ? segment.trackGapSize : trackGap);
            if (s.legacySegments) {
                const auto bounds = lineBounds(previous, a, segment.gapSize / 2, stroke, w);
                if (bounds.y() > bounds.x())
                    line(bounds.x() - stroke / 2, bounds.y() + stroke / 2, s.trackColor);
            } else
                line(previous * w + (previous > 0 ? previousGap * ramp(1 - previous) : 0),
                     a * w - gap * ramp(a),
                     s.trackColor);
            previous    = std::max(previous, b);
            previousGap = gap;
            if (b <= a) continue;
            const auto activeGap =
                std::max(0.0, s.legacySegments ? segment.gapSize / 2 : segment.gapSize);
            const auto legacyBounds =
                s.legacySegments ? lineBounds(a, b, segment.gapSize / 2, stroke, w) : QPointF {};
            if (s.legacySegments && legacyBounds.y() <= legacyBounds.x()) continue;
            const auto activeStart =
                s.legacySegments ? legacyBounds.x() - stroke / 2 : a * w + activeGap * ramp(a);
            const auto activeEnd =
                s.legacySegments ? legacyBounds.y() + stroke / 2 : b * w - activeGap * ramp(1 - b);
            const auto x0 = activeStart + stroke / 2;
            const auto x1 = activeEnd - stroke / 2;
            if (! wave || x1 <= x0) {
                if (wave && activeEnd > activeStart && activeEnd - activeStart < stroke) {
                    const auto phase    = s.phase - std::floor(s.phase);
                    const auto fraction = ((activeStart + activeEnd) / 2 / w + phase / m_cycles) *
                                          qreal(m_cycles) / (m_cycles + 1);
                    auto       center   = m_wave.location(fraction * m_wave.length()).point;
                    center.setX(center.x() - phase * m_actualWavelength);
                    center.setY(y + center.y() * s.waveAmplitude * s.amplitudeFraction);
                    QPainterPath p;
                    const auto   diameter = activeEnd - activeStart;
                    p.addEllipse(center, diameter / 2, diameter / 2);
                    append(p, segment.color, 0);
                } else
                    line(activeStart, activeEnd, segment.color);
                continue;
            }
            const auto phase  = s.phase - std::floor(s.phase);
            const auto offset = phase / m_cycles;
            const auto ratio  = qreal(m_cycles) / (m_cycles + 1);
            auto       path   = m_wave.segment((x0 / w + offset) * ratio * m_wave.length(),
                                               (x1 / w + offset) * ratio * m_wave.length());
            QTransform transform;
            transform.translate(-phase * m_actualWavelength, y);
            transform.scale(1, s.waveAmplitude * s.amplitudeFraction);
            append(transform.map(path), segment.color, stroke);
        }
        if (s.legacySegments) {
            const auto bounds = lineBounds(previous, 1, previousGap, stroke, w);
            if (bounds.y() > bounds.x())
                line(bounds.x() - stroke / 2, bounds.y() + stroke / 2, s.trackColor);
        } else
            line(previous * w + (previous > 0 ? previousGap * ramp(1 - previous) : 0),
                 w,
                 s.trackColor);
        if (s.stopVisible && ! s.indeterminate && s.position < 1 && drain < 0.67) {
            const auto   d = std::min(4.0, w);
            QPainterPath p;
            p.addEllipse(QPointF(w - d / 2, y), d / 2, d / 2);
            append(p, s.stopColor, 0);
        }
    } else {
        if (m_radius <= 0) return result;
        QTransform transform;
        transform.translate(s.legacyCenter ? m_radius : size.width() / 2,
                            s.legacyCenter ? m_radius : size.height() / 2);
        transform.rotate(s.startAngle + s.rotation);
        const auto circumference = m_circle.length() / 2;
        auto       arc           = [&](qreal start, qreal extent, QColor color, bool active) {
            extent = std::clamp(extent, 0.0, 1.0);
            if (extent <= 0) return;
            const bool full = extent >= 1 - 1e-9;
            const auto cap  = full || s.legacySegments ? 0 : stroke / (tau * m_radius) / 2;
            if (extent <= 2 * cap) {
                const auto mid = start + extent / 2;
                auto center    = m_circle.location((mid - std::floor(mid)) * circumference).point;
                if (active && wave) {
                    const auto phase    = (s.phase - std::floor(s.phase)) / m_cycles;
                    const auto fraction = mid + phase - std::floor(mid + phase);
                    center              = m_wave.location(fraction * m_wave.length() / 2).point;
                    QTransform reverse;
                    reverse.rotate(-phase * 360);
                    center = reverse.map(center);
                }
                QPainterPath p;
                const auto   d = extent * tau * m_radius;
                p.addEllipse(center, d / 2, d / 2);
                append(transform.map(p), color, 0);
                return;
            }
            auto path = m_circle.cyclicSegment((start + cap) * circumference,
                                               (extent - 2 * cap) * circumference);
            if (active && wave) {
                const auto phase      = (s.phase - std::floor(s.phase)) / m_cycles;
                const auto waveCircle = m_wave.length() / 2;
                path                  = m_wave.cyclicSegment((start + cap + phase) * waveCircle,
                                                             (extent - 2 * cap) * waveCircle);
                QTransform reverse;
                reverse.rotate(-phase * 360);
                path = reverse.map(path);
            }
            if (full) path.closeSubpath();
            append(transform.map(path), color, stroke);
        };
        auto segments = s.segments;
        if (s.legacySegments)
            for (auto& segment : segments) {
                segment.start /= 360;
                segment.end /= 360;
            }
        const bool legacyGap =
            ! s.indeterminate && ! s.explicitTrack && s.gapAngle >= 0 && s.position < 1;
        if (legacyGap && ! segments.empty()) {
            segments.front().end = std::max(0.0, s.position - 2 * s.gapAngle / 360) * (1 - drain);
            arc(segments.front().end + s.gapAngle / 360,
                (1 - s.position) * (1 - drain),
                s.trackColor,
                false);
        } else if (s.explicitTrack)
            arc(s.trackStart / 360, (s.trackSweep / 360), s.trackColor, false);
        else if (! s.segments.empty()) {
            const auto& segment = s.segments.front();
            const auto  extent  = unit(segment.end - segment.start);
            const auto  gap =
                s.gapAngle >= 0 ? s.gapAngle / 360 : std::max(0.0, s.gapSize) / (tau * m_radius);
            const auto adjusted = gap * ramp(extent) * ramp(1 - extent);
            arc(segment.end + adjusted,
                std::max(0.0, 1 - extent - 2 * adjusted) * (1 - drain),
                s.trackColor,
                false);
        } else
            arc(0, 1 - drain, s.trackColor, false);
        for (const auto& segment : segments)
            arc(segment.start, unit(segment.end - segment.start), segment.color, true);
    }
    return result;
}
} // namespace qml_material
