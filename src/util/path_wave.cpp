#include "qml_material/util/path_wave.hpp"

#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <algorithm>
#include <cmath>

namespace
{

// Local copy of QQuickPath's inline helper at qquickpath.cpp:1360.
inline QPointF position_for_curve(const QQuickPathData& data, const QPointF& prev) {
    QQuickCurve* curve  = data.curves.at(data.index);
    bool         is_end = data.index == data.curves.size() - 1;
    return QPointF(curve->hasRelativeX() ? prev.x() + curve->relativeX()
                   : (! is_end || curve->hasX()) ? curve->x()
                                                 : data.endPoint.x(),
                   curve->hasRelativeY() ? prev.y() + curve->relativeY()
                   : (! is_end || curve->hasY()) ? curve->y()
                                                 : data.endPoint.y());
}

struct Cubic {
    QPointF p0, p1, p2, p3;
};

// De Casteljau split. Returns the first sub-cubic spanning [0, t].
inline Cubic split_first_half(const Cubic& c, qreal t) {
    const QPointF q01 = c.p0 + (c.p1 - c.p0) * t;
    const QPointF q12 = c.p1 + (c.p2 - c.p1) * t;
    const QPointF q23 = c.p2 + (c.p3 - c.p2) * t;
    const QPointF r01 = q01 + (q12 - q01) * t;
    const QPointF r12 = q12 + (q23 - q12) * t;
    const QPointF s   = r01 + (r12 - r01) * t;
    return { c.p0, q01, r01, s };
}

// De Casteljau split. Returns the second sub-cubic spanning [t, 1].
inline Cubic split_second_half(const Cubic& c, qreal t) {
    const QPointF q01 = c.p0 + (c.p1 - c.p0) * t;
    const QPointF q12 = c.p1 + (c.p2 - c.p1) * t;
    const QPointF q23 = c.p2 + (c.p3 - c.p2) * t;
    const QPointF r01 = q01 + (q12 - q01) * t;
    const QPointF r12 = q12 + (q23 - q12) * t;
    const QPointF s   = r01 + (r12 - r01) * t;
    return { s, r12, q23, c.p3 };
}

} // namespace

namespace qml_material
{

PathWave::PathWave(QObject* parent): QQuickCurve(parent) {}
PathWave::~PathWave() = default;

auto PathWave::amplitude() const noexcept -> qreal { return m_amplitude; }
auto PathWave::wave_length() const noexcept -> qreal { return m_wave_length; }
auto PathWave::phase() const noexcept -> qreal { return m_phase; }
auto PathWave::smoothness() const noexcept -> qreal { return m_smoothness; }

void PathWave::set_amplitude(qreal v) {
    if (qFuzzyCompare(m_amplitude, v)) return;
    m_amplitude = v;
    emit amplitudeChanged();
    emit changed();
}

void PathWave::set_wave_length(qreal v) {
    if (qFuzzyCompare(m_wave_length, v)) return;
    m_wave_length = v;
    emit waveLengthChanged();
    emit changed();
}

void PathWave::set_phase(qreal v) {
    if (qFuzzyCompare(m_phase, v)) return;
    m_phase = v;
    emit phaseChanged();
    emit changed();
}

void PathWave::set_smoothness(qreal v) {
    if (qFuzzyCompare(m_smoothness, v)) return;
    m_smoothness = v;
    emit smoothnessChanged();
    emit changed();
}

void PathWave::addToPath(QPainterPath& path, const QQuickPathData& data) {
    const QPointF logical_start = path.currentPosition();
    const QPointF logical_end   = position_for_curve(data, logical_start);

    const qreal length = QLineF(logical_start, logical_end).length();
    if (length < 1e-3 || m_wave_length <= 0.0 || m_amplitude <= 0.0) {
        path.lineTo(logical_end);
        return;
    }

    const QPointF delta = logical_end - logical_start;
    const QPointF axis(delta.x() / length, delta.y() / length);
    const QPointF normal(-axis.y(), axis.x());

    const qreal wl       = m_wave_length;
    const qreal half_wl  = wl / 2.0;
    const qreal s        = m_smoothness;
    const qreal amp      = m_amplitude;
    const qreal ctrl_len = half_wl * s;

    qreal       phase_frac = m_phase - std::floor(m_phase);
    const qreal phase_off  = phase_frac * wl;

    auto pos_at = [&](qreal pixel_arc, qreal signed_v) -> QPointF {
        const qreal arc_pos = pixel_arc - phase_off;
        return logical_start + axis * arc_pos + normal * (signed_v * amp);
    };
    auto tan_at = [&](qreal /*pixel_arc*/, qreal /*signed_v*/) -> QPointF { return axis; };

    const int cycle_count =
        std::max(1, static_cast<int>(std::ceil(length / wl)) + 1);

    bool started = false;
    auto emit_with_truncation =
        [&](const QPointF& a_pos, const QPointF& a_tan, qreal arc_a,
            const QPointF& b_pos, const QPointF& b_tan, qreal arc_b) {
        if (arc_b < 0.0 || arc_a > length) return false;

        Cubic cubic { a_pos, a_pos + a_tan * ctrl_len, b_pos - b_tan * ctrl_len, b_pos };
        qreal cur_a = arc_a;

        if (cur_a < 0.0) {
            const qreal t = (0.0 - cur_a) / (arc_b - cur_a);
            cubic         = split_second_half(cubic, t);
            cur_a         = 0.0;
        }
        if (arc_b > length) {
            const qreal t = (length - cur_a) / (arc_b - cur_a);
            cubic         = split_first_half(cubic, t);
        }

        if (! started) {
            path.moveTo(cubic.p0);
            started = true;
        }
        path.cubicTo(cubic.p1, cubic.p2, cubic.p3);
        return arc_b > length;  // we've crossed the end — caller should stop
    };

    for (int i = 0; i < cycle_count; ++i) {
        const qreal base = i * wl;
        const qreal a    = base - phase_off;
        const qreal b    = base + half_wl - phase_off;
        const qreal c    = base + wl - phase_off;
        // peak (+amp) → trough (−amp)
        if (emit_with_truncation(pos_at(base, 1.0), tan_at(base, 1.0), a,
                                 pos_at(base + half_wl, -1.0),
                                 tan_at(base + half_wl, -1.0), b))
            return;
        // trough (−amp) → peak (+amp)
        if (emit_with_truncation(pos_at(base + half_wl, -1.0),
                                 tan_at(base + half_wl, -1.0), b,
                                 pos_at(base + wl, 1.0), tan_at(base + wl, 1.0), c))
            return;
    }
}

} // namespace qml_material
