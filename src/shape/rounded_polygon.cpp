#include "qml_material/shape/rounded_polygon.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace qml_material
{
namespace
{
constexpr qreal epsilon = 1e-4;
qreal           dot(QPointF a, QPointF b) { return QPointF::dotProduct(a, b); }
qreal           length(QPointF p) { return std::hypot(p.x(), p.y()); }
QPointF         unit(QPointF p) {
    const auto d = length(p);
    return d > 0 ? p / d : QPointF();
}
QPointF rotate90(QPointF p) { return { -p.y(), p.x() }; }
QPointF lerp(QPointF a, QPointF b, qreal t) { return a * (1 - t) + b * t; }
bool    finite(QPointF p) { return std::isfinite(p.x()) && std::isfinite(p.y()); }

struct RoundedCorner {
    QPointF p0, p1, p2, d1, d2;
    qreal   radius, smoothing, roundCut = 0;

    RoundedCorner(QPointF prev, RoundedPolygon::Vertex vertex, QPointF next)
        : p0(prev),
          p1(vertex.position),
          p2(next),
          d1(unit(prev - p1)),
          d2(unit(next - p1)),
          radius(vertex.rounding.radius),
          smoothing(vertex.rounding.smoothing) {
        const auto cosine = std::clamp(dot(d1, d2), qreal(-1), qreal(1));
        const auto sine   = std::sqrt(1 - cosine * cosine);
        if (length(prev - p1) > 0 && length(next - p1) > 0 && sine > 1e-3)
            roundCut = radius * (cosine + 1) / sine;
    }
    qreal cut() const { return (1 + smoothing) * roundCut; }
    qreal actualSmoothing(qreal allowed) const {
        if (allowed > cut()) return smoothing;
        if (allowed > roundCut && cut() > roundCut)
            return smoothing * (allowed - roundCut) / (cut() - roundCut);
        return 0;
    }
    Cubic flank(qreal actualCut, qreal smooth, QPointF side, QPointF intersection,
                QPointF otherIntersection, QPointF center, qreal r) const {
        const auto direction = unit(side - p1);
        const auto start     = p1 + direction * actualCut * (1 + smooth);
        const auto p         = lerp(intersection, (intersection + otherIntersection) / 2, smooth);
        const auto end       = center + unit(p - center) * r;
        const auto tangentNormal = rotate90(rotate90(end - center));
        const auto den           = dot(direction, tangentNormal);
        const auto num           = dot(end - side, tangentNormal);
        const auto anchor = std::abs(den) < epsilon || std::abs(den) < epsilon * std::abs(num)
                                ? intersection
                                : side + direction * (num / den);
        return { start, (start + anchor * 2) / 3, anchor, end };
    }
    std::vector<Cubic> cubics(qreal allowed0, qreal allowed1) const {
        const auto allowed = std::min(allowed0, allowed1);
        if (roundCut < epsilon || allowed < epsilon || radius < epsilon)
            return { Cubic::line(p1, p1) };
        const auto actualCut = std::min(allowed, roundCut);
        const auto r         = radius * actualCut / roundCut;
        const auto center    = p1 + unit(d1 + d2) * std::hypot(r, actualCut);
        const auto i0 = p1 + d1 * actualCut, i1 = p1 + d2 * actualCut;
        const auto f0 = flank(actualCut, actualSmoothing(allowed0), p0, i0, i1, center, r);
        const auto f1 =
            flank(actualCut, actualSmoothing(allowed1), p2, i1, i0, center, r).reversed();
        return { f0, Cubic::arc(center, f0.anchor1, f1.anchor0), f1 };
    }
};

QPainterPath toPath(const std::vector<Cubic>& cubics) {
    QPainterPath path;
    if (cubics.empty()) return path;
    path.moveTo(cubics.front().anchor0);
    for (const auto& c : cubics) path.cubicTo(c.control0, c.control1, c.anchor1);
    path.closeSubpath();
    return path;
}
} // namespace

QPointF Cubic::point(qreal t) const {
    const auto u = 1 - t;
    return anchor0 * (u * u * u) + control0 * (3 * t * u * u) + control1 * (3 * t * t * u) +
           anchor1 * (t * t * t);
}
std::pair<Cubic, Cubic> Cubic::split(qreal t) const {
    const auto a = lerp(anchor0, control0, t), b = lerp(control0, control1, t),
               c = lerp(control1, anchor1, t);
    const auto d = lerp(a, b, t), e = lerp(b, c, t), p = lerp(d, e, t);
    return { { anchor0, a, d, p }, { p, e, c, anchor1 } };
}
Cubic Cubic::reversed() const { return { anchor1, control1, control0, anchor0 }; }
Cubic Cubic::transformed(const QTransform& t) const {
    return { t.map(anchor0), t.map(control0), t.map(control1), t.map(anchor1) };
}
Cubic Cubic::line(QPointF from, QPointF to) {
    return { from, lerp(from, to, 1.0 / 3), lerp(from, to, 2.0 / 3), to };
}
Cubic Cubic::arc(QPointF center, QPointF from, QPointF to) {
    const auto a = unit(from - center), b = unit(to - center);
    const auto cosine = std::clamp(dot(a, b), qreal(-1), qreal(1));
    if (cosine > .999) return line(from, to);
    const auto k = length(from - center) * 4 / 3 *
                   (std::sqrt(2 * (1 - cosine)) - std::sqrt(1 - cosine * cosine)) / (1 - cosine) *
                   (dot(rotate90(a), to - center) >= 0 ? 1 : -1);
    return { from, from + rotate90(a) * k, to - rotate90(b) * k, to };
}

RoundedPolygon::RoundedPolygon(const std::vector<Vertex>& vertices, QPointF center)
    : m_center(center) {
    const auto n = vertices.size();
    if (n < 3 || ! finite(center))
        throw std::invalid_argument("Invalid polygon vertices or center");
    std::vector<RoundedCorner> corners;
    for (size_t i = 0; i < n; ++i) {
        const auto& v = vertices[i];
        if (! finite(v.position) || ! std::isfinite(v.rounding.radius) || v.rounding.radius < 0 ||
            ! std::isfinite(v.rounding.smoothing) || v.rounding.smoothing < 0 ||
            v.rounding.smoothing > 1)
            throw std::invalid_argument("Invalid vertex rounding");
        corners.emplace_back(vertices[(i + n - 1) % n].position, v, vertices[(i + 1) % n].position);
    }
    std::vector<std::pair<qreal, qreal>> adjustments;
    for (size_t i = 0; i < n; ++i) {
        const auto j       = (i + 1) % n;
        const auto side    = length(vertices[i].position - vertices[j].position);
        const auto rounded = corners[i].roundCut + corners[j].roundCut;
        const auto total   = corners[i].cut() + corners[j].cut();
        adjustments.emplace_back(rounded > side ? side / rounded : 1,
                                 rounded > side ? 0
                                 : total > side ? (side - rounded) / (total - rounded)
                                                : 1);
    }
    std::vector<std::vector<Cubic>> curves;
    for (size_t i = 0; i < n; ++i) {
        auto allowed = [&](size_t side) {
            const auto [roundRatio, smoothRatio] = adjustments[side];
            return corners[i].roundCut * roundRatio +
                   (corners[i].cut() - corners[i].roundCut) * smoothRatio;
        };
        curves.push_back(corners[i].cubics(allowed((i + n - 1) % n), allowed(i)));
    }
    for (size_t i = 0; i < n; ++i) {
        const auto prev = vertices[(i + n - 1) % n].position, curr = vertices[i].position,
                   next = vertices[(i + 1) % n].position;
        m_features.push_back({ curves[i], true, dot(rotate90(curr - prev), next - curr) > 0 });
        m_features.push_back(
            { { Cubic::line(curves[i].back().anchor1, curves[(i + 1) % n].front().anchor0) },
              false,
              false });
    }
}

std::vector<Cubic> RoundedPolygon::cubics() const {
    std::vector<Cubic> out;
    const auto         append = [&](const Cubic& c) {
        const auto delta = c.anchor1 - c.anchor0;
        if (std::abs(delta.x()) >= epsilon || std::abs(delta.y()) >= epsilon)
            out.push_back(c);
        else if (! out.empty())
            out.back().anchor1 = c.anchor1;
    };
    const bool splitFirst = ! m_features.empty() && m_features.front().cubics.size() == 3;
    std::pair<Cubic, Cubic> halves;
    if (splitFirst) {
        halves = m_features.front().cubics[1].split(.5);
        append(halves.second);
        append(m_features.front().cubics[2]);
    }
    for (size_t i = splitFirst ? 1 : 0; i < m_features.size(); ++i)
        for (const auto& c : m_features[i].cubics) append(c);
    if (splitFirst) {
        append(m_features.front().cubics[0]);
        append(halves.first);
    }
    if (! out.empty()) out.back().anchor1 = out.front().anchor0;
    return out;
}
QPainterPath   RoundedPolygon::path() const { return toPath(cubics()); }
RoundedPolygon RoundedPolygon::transformed(const QTransform& transform) const {
    auto result     = *this;
    result.m_center = transform.map(m_center);
    for (auto& f : result.m_features) {
        for (auto& c : f.cubics) c = c.transformed(transform);
    }
    return result;
}
RoundedPolygon RoundedPolygon::normalized(bool radial) const {
    QRectF bounds = path().controlPointRect();
    if (radial) {
        qreal radius = 0;
        for (const auto& c : cubics())
            radius =
                std::max({ radius, length(c.anchor0 - m_center), length(c.point(.5) - m_center) });
        bounds = QRectF(m_center - QPointF(radius, radius), QSizeF(2 * radius, 2 * radius));
    }
    const auto side = std::max(bounds.width(), bounds.height());
    if (side <= 0) return *this;
    QTransform t;
    t.translate(.5, .5);
    t.scale(1 / side, 1 / side);
    t.translate(-bounds.center().x(), -bounds.center().y());
    return transformed(t);
}

} // namespace qml_material
