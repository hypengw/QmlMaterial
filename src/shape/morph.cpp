#include "qml_material/shape/rounded_polygon.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace qml_material
{
namespace
{
constexpr qreal epsilon = 1e-4;
qreal           wrap(qreal p) { return p - std::floor(p); }
qreal           distance(QPointF p) { return std::hypot(p.x(), p.y()); }
qreal           progressDistance(qreal a, qreal b) {
    const auto d = std::abs(a - b);
    return std::min(d, 1 - d);
}
bool inRange(qreal p, qreal a, qreal b) { return a <= b ? p >= a && p <= b : p >= a || p <= b; }

std::pair<qreal, qreal> measure(const Cubic& cubic,
                                qreal        threshold = std::numeric_limits<qreal>::infinity()) {
    qreal total = 0;
    auto  prev  = cubic.anchor0;
    for (int i = 1; i <= 3; ++i) {
        const qreal t       = qreal(i) / 3;
        const auto  point   = cubic.point(t);
        const auto  segment = distance(point - prev);
        if (segment > 0 && segment >= threshold - total)
            return { t - (1 - (threshold - total) / segment) / 3, threshold };
        total += segment;
        prev = point;
    }
    return { 1, total };
}

struct MeasuredCubic {
    Cubic                                   cubic;
    qreal                                   start, end;
    std::pair<MeasuredCubic, MeasuredCubic> split(qreal progress) const {
        progress            = std::clamp(progress, start, end);
        const auto relative = (progress - start) / (end - start);
        const auto t        = measure(cubic, relative * measure(cubic).second).first;
        const auto [a, b]   = cubic.split(t);
        return { { a, start, progress }, { b, progress, end } };
    }
};
struct MeasuredFeature {
    qreal   progress;
    QPointF point;
    bool    convex;
};
struct MeasuredPolygon {
    std::vector<MeasuredCubic>   cubics;
    std::vector<MeasuredFeature> features;
    explicit MeasuredPolygon(const RoundedPolygon& polygon) {
        qreal total = 0;
        for (const auto& feature : polygon.features()) {
            const auto middle = feature.cubics.size() / 2;
            for (size_t i = 0; i < feature.cubics.size(); ++i) {
                const auto& c    = feature.cubics[i];
                const auto  size = measure(c).second;
                if (feature.corner && i == middle)
                    features.push_back(
                        { total + size / 2,
                          (feature.cubics.front().anchor0 + feature.cubics.back().anchor1) / 2,
                          feature.convex });
                cubics.push_back({ c, total, total + size });
                total += size;
            }
        }
        if (total <= 0) throw std::invalid_argument("Cannot morph an empty polygon");
        for (auto& f : features) f.progress = wrap(f.progress / total);
        for (auto& c : cubics) {
            c.start /= total;
            c.end /= total;
        }
        removeEmpty();
    }
    void removeEmpty() {
        std::erase_if(cubics, [](const auto& c) {
            return c.end - c.start <= epsilon;
        });
        qreal start = 0;
        for (auto& c : cubics) {
            c.start = start;
            start   = c.end;
        }
        if (! cubics.empty()) cubics.back().end = 1;
    }
    void shift(qreal cut) {
        if (cut < epsilon) return;
        const auto it     = std::find_if(cubics.begin(), cubics.end(), [cut](const auto& c) {
            return cut >= c.start && cut <= c.end;
        });
        const auto index  = size_t(it - cubics.begin());
        const auto [a, b] = it->split(cut);
        std::vector<MeasuredCubic> shifted;
        shifted.push_back({ b.cubic, 0, b.end - cut });
        for (size_t i = 1; i < cubics.size(); ++i) {
            const auto& c = cubics[(index + i) % cubics.size()];
            shifted.push_back({ c.cubic, shifted.back().end, wrap(c.end - cut) });
        }
        shifted.push_back({ a.cubic, shifted.back().end, 1 });
        cubics = std::move(shifted);
        removeEmpty();
    }
};

class FeatureMapping {
public:
    FeatureMapping(const std::vector<MeasuredFeature>& from,
                   const std::vector<MeasuredFeature>& to) {
        struct Candidate {
            size_t from, to;
            qreal  distance;
        };
        std::vector<Candidate> candidates;
        for (size_t i = 0; i < from.size(); ++i)
            for (size_t j = 0; j < to.size(); ++j)
                if (from[i].convex == to[j].convex) {
                    const auto d = from[i].point - to[j].point;
                    candidates.push_back({ i, j, QPointF::dotProduct(d, d) });
                }
        std::stable_sort(candidates.begin(), candidates.end(), [](auto a, auto b) {
            return a.distance < b.distance;
        });
        std::vector<bool> usedFrom(from.size()), usedTo(to.size());
        for (const auto& c : candidates) {
            if (usedFrom[c.from] || usedTo[c.to]) continue;
            const auto a = from[c.from].progress, b = to[c.to].progress;
            const auto it =
                std::lower_bound(m_pairs.begin(), m_pairs.end(), a, [](auto pair, qreal p) {
                    return pair.first < p;
                });
            const auto index = size_t(it - m_pairs.begin()), n = m_pairs.size();
            if (n > 0) {
                const auto before = m_pairs[(index + n - 1) % n], after = m_pairs[index % n];
                if (progressDistance(a, before.first) < epsilon ||
                    progressDistance(a, after.first) < epsilon ||
                    progressDistance(b, before.second) < epsilon ||
                    progressDistance(b, after.second) < epsilon)
                    continue;
                if (n > 1 && ! inRange(b, before.second, after.second)) continue;
            }
            m_pairs.insert(it, { a, b });
            usedFrom[c.from] = usedTo[c.to] = true;
        }
        if (m_pairs.empty())
            m_pairs = { { 0, 0 }, { .5, .5 } };
        else if (m_pairs.size() == 1) {
            const auto p = m_pairs.front();
            m_pairs.emplace_back(wrap(p.first + .5), wrap(p.second + .5));
            std::sort(m_pairs.begin(), m_pairs.end());
        }
    }
    qreal map(qreal x, bool reverse = false) const {
        for (size_t i = 0; i < m_pairs.size(); ++i) {
            auto [a, b] = m_pairs[i];
            auto [c, d] = m_pairs[(i + 1) % m_pairs.size()];
            if (reverse) {
                std::swap(a, b);
                std::swap(c, d);
            }
            if (! inRange(x, a, c)) continue;
            const auto span = wrap(c - a);
            const auto t    = span < .001 ? .5 : wrap(x - a) / span;
            return wrap(b + wrap(d - b) * t);
        }
        throw std::logic_error("Invalid feature mapping");
    }

private:
    std::vector<std::pair<qreal, qreal>> m_pairs;
};
} // namespace

Morph::Morph(const RoundedPolygon& from, const RoundedPolygon& to) {
    MeasuredPolygon      a(from), b(to);
    const FeatureMapping mapping(a.features, b.features);
    const auto           cut = mapping.map(0);
    b.shift(cut);
    size_t i = 0, j = 0;
    auto   c1 = a.cubics.front(), c2 = b.cubics.front();
    while (i < a.cubics.size() && j < b.cubics.size()) {
        const auto end1 = i + 1 == a.cubics.size() ? 1 : c1.end;
        const auto end2 = j + 1 == b.cubics.size() ? 1 : mapping.map(wrap(c2.end + cut), true);
        const auto end  = std::min(end1, end2);
        Cubic      s1, s2;
        if (end1 > end + 1e-6) {
            const auto parts = c1.split(end);
            s1               = parts.first.cubic;
            c1               = parts.second;
        } else {
            s1 = c1.cubic;
            if (++i < a.cubics.size()) c1 = a.cubics[i];
        }
        if (end2 > end + 1e-6) {
            const auto parts = c2.split(wrap(mapping.map(end) - cut));
            s2               = parts.first.cubic;
            c2               = parts.second;
        } else {
            s2 = c2.cubic;
            if (++j < b.cubics.size()) c2 = b.cubics[j];
        }
        m_match.emplace_back(s1, s2);
    }
    if (i != a.cubics.size() || j != b.cubics.size())
        throw std::logic_error("Incomplete cubic matching");
}

std::vector<Cubic> Morph::cubics(qreal progress) const {
    if (! std::isfinite(progress)) throw std::invalid_argument("Invalid morph progress");
    std::vector<Cubic> out;
    out.reserve(m_match.size());
    auto lerp = [progress](QPointF a, QPointF b) {
        return a * (1 - progress) + b * progress;
    };
    for (const auto& [a, b] : m_match)
        out.push_back({ lerp(a.anchor0, b.anchor0),
                        lerp(a.control0, b.control0),
                        lerp(a.control1, b.control1),
                        lerp(a.anchor1, b.anchor1) });
    // Dropped zero-length curves may leave subpixel gaps at feature boundaries.
    for (size_t i = 1; i < out.size(); ++i) out[i].anchor0 = out[i - 1].anchor1;
    if (! out.empty()) out.back().anchor1 = out.front().anchor0;
    return out;
}
QPainterPath Morph::path(qreal progress) const {
    const auto   curves = cubics(progress);
    QPainterPath path;
    if (curves.empty()) return path;
    path.moveTo(curves.front().anchor0);
    for (const auto& c : curves) path.cubicTo(c.control0, c.control1, c.anchor1);
    path.closeSubpath();
    return path;
}
} // namespace qml_material
