#include "qml_material/path/path_arc_wave.hpp"

#include <QPainterPath>
#include <QPointF>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace
{

struct Cubic {
    QPointF p0, p1, p2, p3;
};

inline Cubic split_first_half(const Cubic& c, qreal t) {
    const QPointF q01 = c.p0 + (c.p1 - c.p0) * t;
    const QPointF q12 = c.p1 + (c.p2 - c.p1) * t;
    const QPointF q23 = c.p2 + (c.p3 - c.p2) * t;
    const QPointF r01 = q01 + (q12 - q01) * t;
    const QPointF r12 = q12 + (q23 - q12) * t;
    const QPointF s   = r01 + (r12 - r01) * t;
    return { c.p0, q01, r01, s };
}

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

PathArcWave::PathArcWave(QObject* parent): QQuickCurve(parent) {}
PathArcWave::~PathArcWave() = default;

auto PathArcWave::center_x() const noexcept -> qreal { return m_center_x; }
auto PathArcWave::center_y() const noexcept -> qreal { return m_center_y; }
auto PathArcWave::radius() const noexcept -> qreal { return m_radius; }
auto PathArcWave::start_angle() const noexcept -> qreal { return m_start_angle; }
auto PathArcWave::sweep_angle() const noexcept -> qreal { return m_sweep_angle; }
auto PathArcWave::amplitude() const noexcept -> qreal { return m_amplitude; }
auto PathArcWave::wave_length() const noexcept -> qreal { return m_wave_length; }
auto PathArcWave::phase() const noexcept -> qreal { return m_phase; }
auto PathArcWave::smoothness() const noexcept -> qreal { return m_smoothness; }

void PathArcWave::set_center_x(qreal v) {
    if (qFuzzyCompare(m_center_x, v)) return;
    m_center_x = v;
    emit centerXChanged();
    emit changed();
}
void PathArcWave::set_center_y(qreal v) {
    if (qFuzzyCompare(m_center_y, v)) return;
    m_center_y = v;
    emit centerYChanged();
    emit changed();
}
void PathArcWave::set_radius(qreal v) {
    if (qFuzzyCompare(m_radius, v)) return;
    m_radius = v;
    emit radiusChanged();
    emit changed();
}
void PathArcWave::set_start_angle(qreal v) {
    if (qFuzzyCompare(m_start_angle, v)) return;
    m_start_angle = v;
    emit startAngleChanged();
    emit changed();
}
void PathArcWave::set_sweep_angle(qreal v) {
    if (qFuzzyCompare(m_sweep_angle, v)) return;
    m_sweep_angle = v;
    emit sweepAngleChanged();
    emit changed();
}
void PathArcWave::set_amplitude(qreal v) {
    if (qFuzzyCompare(m_amplitude, v)) return;
    m_amplitude = v;
    emit amplitudeChanged();
    emit changed();
}
void PathArcWave::set_wave_length(qreal v) {
    if (qFuzzyCompare(m_wave_length, v)) return;
    m_wave_length = v;
    emit waveLengthChanged();
    emit changed();
}
void PathArcWave::set_phase(qreal v) {
    if (qFuzzyCompare(m_phase, v)) return;
    m_phase = v;
    emit phaseChanged();
    emit changed();
}
void PathArcWave::set_smoothness(qreal v) {
    if (qFuzzyCompare(m_smoothness, v)) return;
    m_smoothness = v;
    emit smoothnessChanged();
    emit changed();
}

void PathArcWave::addToPath(QPainterPath& path, const QQuickPathData&) {
    if (m_radius <= 0.0 || m_wave_length <= 0.0 || qFuzzyIsNull(m_sweep_angle)) {
        return;
    }

    const qreal start_rad = qDegreesToRadians(m_start_angle);
    const qreal sweep_rad = qDegreesToRadians(m_sweep_angle);
    const qreal arc_len   = std::abs(sweep_rad) * m_radius;
    const int   dir       = sweep_rad >= 0 ? 1 : -1;

    if (arc_len < 1e-3 || m_amplitude <= 0.0) {
        const qreal x0 = m_center_x - m_radius;
        const qreal y0 = m_center_y - m_radius;
        const qreal sz = m_radius * 2.0;
        path.arcMoveTo(x0, y0, sz, sz, -m_start_angle);
        path.arcTo(x0, y0, sz, sz, -m_start_angle, -m_sweep_angle);
        return;
    }

    const qreal wl       = m_wave_length;
    const qreal half_wl  = wl / 2.0;
    const qreal s        = m_smoothness;
    const qreal amp      = m_amplitude;
    const qreal ctrl_len = half_wl * s;

    qreal       phase_frac = m_phase - std::floor(m_phase);
    const qreal phase_off  = phase_frac * wl;

    auto pos_at = [&](qreal pixel_arc, qreal signed_v) -> QPointF {
        const qreal arc_pos = pixel_arc - phase_off;
        // angle = -(start + arc_pos*dir/r), pos = (cx + r*cos(angle),
        // cy - r*sin(angle))
        const qreal angle = -(start_rad + (arc_pos / m_radius) * dir);
        const qreal r     = m_radius + signed_v * amp;
        return QPointF(m_center_x + r * std::cos(angle),
                       m_center_y - r * std::sin(angle));
    };
    auto tan_at = [&](qreal pixel_arc, qreal /*signed_v*/) -> QPointF {
        const qreal arc_pos = pixel_arc - phase_off;
        const qreal angle   = -(start_rad + (arc_pos / m_radius) * dir);
        // d/d(arc_pos) of pos on the centerline circle:
        return QPointF(std::sin(angle) * dir, std::cos(angle) * dir);
    };

    const int cycle_count =
        std::max(1, static_cast<int>(std::ceil(arc_len / wl)) + 1);

    bool started = false;
    auto emit_with_truncation =
        [&](const QPointF& a_pos, const QPointF& a_tan, qreal arc_a,
            const QPointF& b_pos, const QPointF& b_tan, qreal arc_b) {
        if (arc_b < 0.0 || arc_a > arc_len) return false;

        Cubic cubic { a_pos, a_pos + a_tan * ctrl_len, b_pos - b_tan * ctrl_len, b_pos };
        qreal cur_a = arc_a;

        if (cur_a < 0.0) {
            const qreal t = (0.0 - cur_a) / (arc_b - cur_a);
            cubic         = split_second_half(cubic, t);
            cur_a         = 0.0;
        }
        if (arc_b > arc_len) {
            const qreal t = (arc_len - cur_a) / (arc_b - cur_a);
            cubic         = split_first_half(cubic, t);
        }

        if (! started) {
            path.moveTo(cubic.p0);
            started = true;
        }
        path.cubicTo(cubic.p1, cubic.p2, cubic.p3);
        return arc_b > arc_len;
    };

    for (int i = 0; i < cycle_count; ++i) {
        const qreal base = i * wl;
        const qreal a    = base - phase_off;
        const qreal b    = base + half_wl - phase_off;
        const qreal c    = base + wl - phase_off;
        if (emit_with_truncation(pos_at(base, 1.0), tan_at(base, 1.0), a,
                                 pos_at(base + half_wl, -1.0),
                                 tan_at(base + half_wl, -1.0), b))
            return;
        if (emit_with_truncation(pos_at(base + half_wl, -1.0),
                                 tan_at(base + half_wl, -1.0), b,
                                 pos_at(base + wl, 1.0), tan_at(base + wl, 1.0), c))
            return;
    }
}

} // namespace qml_material
