#include "qml_material/path/measured_path.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <QtCore/QLineF>

namespace qml_material
{
namespace
{
qreal distance(QPointF a, QPointF b) { return QLineF(a, b).length(); }
bool  finite(QPointF p) { return std::isfinite(p.x()) && std::isfinite(p.y()); }
} // namespace

MeasuredPath::MeasuredPath(std::vector<Cubic> curves, qreal tolerance) {
    tolerance = std::isfinite(tolerance) ? std::max(0.00001, tolerance) : 0.001;
    for (const auto& curve : curves) {
        if (! finite(curve.anchor0) || ! finite(curve.control0) || ! finite(curve.control1) ||
            ! finite(curve.anchor1))
            continue;
        Entry entry { curve, m_length, 0, { { 0, 0 } } };
        std::function<void(const Cubic&, qreal, qreal, int)> measure;
        measure = [&](const Cubic& c, qreal a, qreal b, int depth) {
            const auto chord   = distance(c.anchor0, c.anchor1);
            const auto polygon = distance(c.anchor0, c.control0) +
                                 distance(c.control0, c.control1) + distance(c.control1, c.anchor1);
            // Also bound the inverse parameter lookup for collinear, nonuniform cubics.
            const auto midpointError =
                std::max(distance(c.control0, (2 * c.anchor0 + c.anchor1) / 3),
                         distance(c.control1, (c.anchor0 + 2 * c.anchor1) / 3));
            if (depth < 20 &&
                (polygon - chord > tolerance * (b - a) || midpointError > tolerance)) {
                const auto halves = c.split(0.5);
                measure(halves.first, a, (a + b) / 2, depth + 1);
                measure(halves.second, (a + b) / 2, b, depth + 1);
            } else {
                entry.length += (chord + polygon) / 2;
                entry.samples.push_back({ b, entry.length });
            }
        };
        measure(curve, 0, 1, 0);
        if (entry.length > 0) {
            m_length += entry.length;
            m_entries.push_back(std::move(entry));
        }
    }
}

qreal MeasuredPath::parameter(const Entry& entry, qreal d) const {
    d       = std::clamp(d, qreal(0), entry.length);
    auto it = std::lower_bound(
        entry.samples.begin(), entry.samples.end(), d, [](const Sample& s, qreal value) {
            return s.distance < value;
        });
    if (it == entry.samples.begin()) return 0;
    if (it == entry.samples.end()) return 1;
    const auto& prev = *(it - 1);
    const auto  span = it->distance - prev.distance;
    return span > 0 ? std::lerp(prev.t, it->t, (d - prev.distance) / span) : it->t;
}

MeasuredPath::Location MeasuredPath::location(qreal d) const {
    if (m_entries.empty() || ! std::isfinite(d)) return {};
    d = std::clamp(d, qreal(0), m_length);
    auto it =
        std::lower_bound(m_entries.begin(), m_entries.end(), d, [](const Entry& e, qreal value) {
            return e.start + e.length < value;
        });
    if (it == m_entries.end()) it = m_entries.end() - 1;
    const auto  t = parameter(*it, d - it->start);
    const auto& c = it->curve;
    auto        tangent =
        3 * ((1 - t) * (1 - t) * (c.control0 - c.anchor0) +
             2 * (1 - t) * t * (c.control1 - c.control0) + t * t * (c.anchor1 - c.control1));
    auto norm = std::hypot(tangent.x(), tangent.y());
    if (norm < 1e-12) {
        tangent = c.point(std::min(1.0, t + 0.0001)) - c.point(std::max(0.0, t - 0.0001));
        norm    = std::hypot(tangent.x(), tangent.y());
    }
    return { c.point(t), norm > 0 ? tangent / norm : QPointF {} };
}

QPainterPath MeasuredPath::segment(qreal start, qreal end) const {
    QPainterPath path;
    if (! std::isfinite(start) || ! std::isfinite(end)) return path;
    start = std::clamp(start, qreal(0), m_length);
    end   = std::clamp(end, qreal(0), m_length);
    if (end <= start) return path;
    auto first = std::lower_bound(
        m_entries.begin(), m_entries.end(), start, [](const Entry& entry, qreal value) {
            return entry.start + entry.length <= value;
        });
    for (auto it = first; it != m_entries.end(); ++it) {
        const auto& entry = *it;
        if (entry.start >= end) break;
        const auto a = parameter(entry, start - entry.start);
        const auto b = parameter(entry, end - entry.start);
        if (b <= a) continue;
        auto curve = entry.curve;
        if (b < 1) curve = curve.split(b).first;
        if (a > 0) curve = curve.split(a / b).second;
        if (path.isEmpty()) path.moveTo(curve.anchor0);
        path.cubicTo(curve.control0, curve.control1, curve.anchor1);
    }
    return path;
}

QPainterPath MeasuredPath::cyclicSegment(qreal start, qreal extent) const {
    if (m_length <= 0 || ! std::isfinite(start) || ! std::isfinite(extent) || extent <= 0)
        return {};
    start = std::fmod(start, m_length);
    if (start < 0) start += m_length;
    extent    = std::min(extent, m_length);
    auto path = segment(start, std::min(m_length, start + extent));
    if (start + extent > m_length) path.connectPath(segment(0, start + extent - m_length));
    return path;
}
} // namespace qml_material
