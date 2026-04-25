#include "qml_material/anim/loading_indicator_updator.hpp"

#include <QtCore/QtMath>
#include <QtGui/QPainterPath>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
// ── Animation constants (match MDC-Android) ───────────────────────────────────
constexpr int CONSTANT_ROTATION_PER_SHAPE = 50;
constexpr int EXTRA_ROTATION_PER_SHAPE    = 90;

// ── Spring simulation ─────────────────────────────────────────────────────────
// Analytically models MDC's SpringForce(stiffness=200, dampingRatio=0.6).
// Returns the spring's displacement for a normalised input t ∈ [0, 1],
// where t=1 maps to one shape-step duration (650 ms).
// The value can briefly overshoot 1 — that's intentional (bouncy morph/rotation).
inline double springValue(double t) noexcept {
    // ω₀ = √stiffness = √200 ≈ 14.142
    // ζ  = dampingRatio = 0.6
    // ωd = ω₀ √(1 − ζ²) = 14.142 × 0.8 = 11.314
    constexpr double omega0 = 14.142135623730951;
    constexpr double zeta   = 0.6;
    constexpr double omegaD = 11.313708498984761;
    constexpr double ratio  = zeta * omega0 / omegaD; // ≈ 0.75

    const double rt    = t * 0.65; // map [0,1] → [0, 0.65 s]
    const double decay = std::exp(-zeta * omega0 * rt);
    return 1.0 - decay * (std::cos(omegaD * rt) + ratio * std::sin(omegaD * rt));
}

// ── Path sampling / normalisation ─────────────────────────────────────────────
QPolygonF samplePath(const QPainterPath& path, int n) {
    QPolygonF poly;
    poly.reserve(n);
    for (int i = 0; i < n; ++i) poly.append(path.pointAtPercent(double(i) / n));
    return poly;
}

// Translate so the polygon centre maps to origin, then scale uniformly so the
// farthest sample lies on the unit circle. Mirrors MDC's
// MaterialShapes.normalize(shape, radial=true, RectF(-1,-1,1,1)).
QPolygonF radialNormalize(const QPolygonF& poly, qreal cx = 0, qreal cy = 0) {
    qreal maxDist = 0;
    for (const auto& p : poly) {
        maxDist = std::max(maxDist, std::hypot(p.x() - cx, p.y() - cy));
    }
    const qreal scale = (maxDist > 0) ? (1.0 / maxDist) : 1.0;
    QPolygonF out;
    out.reserve(poly.size());
    for (const auto& p : poly) {
        out.append({ (p.x() - cx) * scale, (p.y() - cy) * scale });
    }
    return out;
}

// ── Rounded-polygon path (MDC CornerRounding, smoothing = 0) ──────────────────
// Each vertex's `radius` is the circular-arc radius (MDC convention). The cut
// distance d_i = r_i / tan(β_i / 2), where β_i is the unsigned angle between
// the incoming and outgoing edge unit vectors. If the two cuts on the same
// edge would overlap, both are scaled down proportionally so their sum equals
// the edge length — this matches MDC's clamping intent and lets a vertex with
// large radius borrow space from a neighbour with small/zero radius.
// Each rounded corner is a cubic-Bézier approximation of the circular arc.
struct CornerVertex {
    QPointF v;       // cartesian on input; transient (angle, dist) inside repeatAround
    qreal   radius;
};

QPainterPath buildRoundedPath(const QList<CornerVertex>& verts) {
    QPainterPath path;
    const int M = verts.size();
    if (M < 3) return path;

    QList<QPointF> dir(M);
    QList<qreal>   len(M);
    for (int i = 0; i < M; ++i) {
        QPointF     d = verts[(i + 1) % M].v - verts[i].v;
        const qreal L = std::hypot(d.x(), d.y());
        if (L > 0) d /= L;
        dir[i] = d;
        len[i] = L;
    }

    QList<qreal> beta(M, 0.0);
    QList<qreal> desired(M, 0.0);
    for (int i = 0; i < M; ++i) {
        const QPointF& vin  = dir[(i + M - 1) % M];
        const QPointF& vout = dir[i];
        const qreal cosb = std::clamp(-(vin.x() * vout.x() + vin.y() * vout.y()), -1.0, 1.0);
        const qreal b    = std::acos(cosb);
        beta[i]          = b;
        if (b > 1e-9 && verts[i].radius > 0) {
            desired[i] = verts[i].radius / std::tan(b * 0.5);
        }
    }

    QList<qreal> edgeScale(M, 1.0);
    for (int i = 0; i < M; ++i) {
        const qreal sum = desired[i] + desired[(i + 1) % M];
        if (sum > len[i] && sum > 0) edgeScale[i] = len[i] / sum;
    }

    QList<qreal> cut(M, 0.0);
    for (int i = 0; i < M; ++i) {
        cut[i] = desired[i] * std::min(edgeScale[(i + M - 1) % M], edgeScale[i]);
    }

    for (int i = 0; i < M; ++i) {
        const QPointF& vin  = dir[(i + M - 1) % M];
        const QPointF& vout = dir[i];
        const QPointF& p1   = verts[i].v;
        const qreal    d    = cut[i];
        const QPointF  s    = p1 - vin * d;
        const QPointF  e    = p1 + vout * d;
        if (i == 0) path.moveTo(s);
        else path.lineTo(s);
        if (d > 0 && beta[i] < M_PI - 1e-6) {
            // Arc spans α = π − β. Cubic-Bézier control distance from each
            // tangent point along its tangent: c = (2/3)·(1 − tan²(α/4))·d.
            // Equivalent to the (4/3)·tan(α/4)·r form, recast in d to avoid
            // a sin(β/2) division as β → π.
            const qreal alpha = M_PI - beta[i];
            const qreal t     = std::tan(alpha * 0.25);
            const qreal c     = (2.0 / 3.0) * (1.0 - t * t) * d;
            path.cubicTo(s + vin * c, e - vout * c, e);
        }
    }
    path.closeSubpath();
    return path;
}

// ── Shape constructors ────────────────────────────────────────────────────────
QPolygonF buildRegularPolygon(int vertices, qreal radius, qreal cornerRadius, qreal rotationRad,
                              int n) {
    if (vertices < 3) return {};
    QList<CornerVertex> verts;
    verts.reserve(vertices);
    for (int i = 0; i < vertices; ++i) {
        const qreal a = rotationRad + (2.0 * M_PI * i / vertices);
        verts.append({ { radius * std::cos(a), radius * std::sin(a) }, cornerRadius });
    }
    return radialNormalize(samplePath(buildRoundedPath(verts), n));
}

QPolygonF buildStarPolygon(int spokes, qreal outerR, qreal innerR, qreal cornerRadius,
                           qreal rotationRad, int n) {
    if (spokes < 2) return {};
    const int           total = spokes * 2;
    QList<CornerVertex> verts;
    verts.reserve(total);
    for (int i = 0; i < total; ++i) {
        const qreal r = (i % 2 == 0) ? outerR : innerR;
        const qreal a = rotationRad + (M_PI * i / spokes);
        verts.append({ { r * std::cos(a), r * std::sin(a) }, cornerRadius });
    }
    return radialNormalize(samplePath(buildRoundedPath(verts), n));
}

QList<CornerVertex> repeatAround(QList<CornerVertex> tmpl, int repeat, qreal cx, qreal cy,
                                 bool mirror) {
    // template -> polar (relative to center)
    for (auto& vr : tmpl) {
        const qreal dx = vr.v.x() - cx;
        const qreal dy = vr.v.y() - cy;
        vr.v           = { std::atan2(dy, dx), std::hypot(dx, dy) };
    }

    QList<CornerVertex> out;
    qreal               spanPerRepeat = 2.0 * M_PI / repeat;
    if (mirror) {
        const int mirroredRepeatCount = repeat * 2;
        spanPerRepeat /= 2.0;
        for (int i = 0; i < mirroredRepeatCount; ++i) {
            const bool reverse = (i & 1) != 0;
            for (int j = 0; j < tmpl.size(); ++j) {
                const int idx = reverse ? tmpl.size() - 1 - j : j;
                if (idx == 0 && reverse) continue; // matches MDC's `idx > 0 || !reverse`
                const auto& tp    = tmpl[idx];
                const qreal angle = spanPerRepeat * i +
                                    (reverse ? spanPerRepeat - tp.v.x() + 2.0 * tmpl[0].v.x()
                                             : tp.v.x());
                out.append({ QPointF(angle, tp.v.y()), tp.radius });
            }
        }
    } else {
        for (int i = 0; i < repeat; ++i) {
            for (const auto& tp : tmpl) {
                const qreal angle = spanPerRepeat * i + tp.v.x();
                out.append({ QPointF(angle, tp.v.y()), tp.radius });
            }
        }
    }

    // polar -> cartesian
    for (auto& vr : out) {
        const qreal angle = vr.v.x();
        const qreal dist  = vr.v.y();
        vr.v              = { dist * std::cos(angle) + cx, dist * std::sin(angle) + cy };
    }
    return out;
}

QPolygonF buildCustomPolygon(QList<CornerVertex> tmpl, int repeat, qreal cx, qreal cy, bool mirror,
                             int n) {
    QList<CornerVertex> verts = repeatAround(std::move(tmpl), repeat, cx, cy, mirror);
    return radialNormalize(samplePath(buildRoundedPath(verts), n), cx, cy);
}

// ── Winding alignment ─────────────────────────────────────────────────────────
// Returns a copy of `target` whose point-winding is rotated so the total
// squared distance to `reference` is minimised.  Per-iteration early-out
// keeps worst-case cost well under O(n²) for typical shapes.
QPolygonF alignTo(const QPolygonF& reference, const QPolygonF& target) {
    const int n = std::min(reference.size(), target.size());
    if (n == 0) return target;

    int   bestK    = 0;
    qreal bestCost = std::numeric_limits<qreal>::max();

    for (int k = 0; k < n; ++k) {
        qreal cost = 0;
        for (int i = 0; i < n; ++i) {
            const QPointF d = reference[i] - target[(i + k) % n];
            cost += d.x() * d.x() + d.y() * d.y();
            if (cost >= bestCost) goto next_k;
        }
        bestCost = cost;
        bestK    = k;
    next_k:;
    }

    if (bestK == 0) return target;

    QPolygonF out;
    out.reserve(n);
    for (int i = 0; i < n; ++i) out.append(target[(i + bestK) % n]);
    return out;
}

} // namespace

namespace qml_material
{

// ── Constructor ───────────────────────────────────────────────────────────────
LoadingIndicatorUpdator::LoadingIndicatorUpdator(QObject* parent)
    : QObject(parent), m_progress(0), m_morph_fraction(0), m_rotation(0) {
    m_predefined_shapes.reserve(SHAPE_COUNT);
    const int N = SAMPLE_COUNT;

    // Shapes approximate the MDC-Android INDETERMINATE_SHAPES sequence
    // (com.google.android.material.shape.MaterialShapes), all normalised to
    // [-1, 1]. Star/regular-polygon shapes are exact ports; shapes built from
    // RoundedPolygon's `customPolygon(...)` (per-vertex CornerRounding with
    // a repeated template) are approximated with simpler primitives — see
    // tests/scenes/loading_indicator_shapes.qml for visual comparison.

    // SOFT_BURST: MDC customPolygon — 2 anchors × 10 spokes, no mirror.
    m_predefined_shapes.append(buildCustomPolygon(
        { { QPointF(0.193, 0.277), 0.053 }, { QPointF(0.176, 0.055), 0.053 } },
        /* repeat */ 10, /* cx */ 0.5, /* cy */ 0.5, /* mirror */ false, N));

    // COOKIE_9: MDC = star(9, 1, 0.8, rounding=0.5), rotated -90°.
    m_predefined_shapes.append(buildStarPolygon(9, 1.0, 0.8, 0.5, -M_PI / 2.0, N));

    // PENTAGON: MDC = customPolygon with 1 anchor (~apex up) and rounding 0.172.
    // A regular pentagon rotated to point up is geometrically equivalent.
    m_predefined_shapes.append(buildRegularPolygon(5, 1.0, 0.172, -M_PI / 2.0, N));

    // PILL: MDC customPolygon — 3 anchors × 2, mirrored.
    m_predefined_shapes.append(buildCustomPolygon(
        { { QPointF(0.961, 0.039), 0.426 },
          { QPointF(1.001, 0.428), 0.0 },
          { QPointF(1.000, 0.609), 1.0 } },
        /* repeat */ 2, /* cx */ 0.5, /* cy */ 0.5, /* mirror */ true, N));

    // SUNNY: MDC = star(8, 1, 0.8, rounding=0.15).
    m_predefined_shapes.append(buildStarPolygon(8, 1.0, 0.8, 0.15, 0.0, N));

    // COOKIE_4: MDC customPolygon — 2 anchors × 4 spokes, no mirror.
    m_predefined_shapes.append(buildCustomPolygon(
        { { QPointF(1.237, 1.236), 0.258 }, { QPointF(0.500, 0.918), 0.233 } },
        /* repeat */ 4, /* cx */ 0.5, /* cy */ 0.5, /* mirror */ false, N));

    { // OVAL: MDC = circle scaled to scaleY = 0.64, then rotated -45°
      // (an ellipse tilted toward the upper-right).
        QPainterPath p;
        p.addEllipse(QPointF(0, 0), 1.0, 0.64);
        QPolygonF       poly = samplePath(p, N);
        constexpr qreal kCos = 0.7071067811865476;  // cos(-45°)
        constexpr qreal kSin = -0.7071067811865475; // sin(-45°)
        for (auto& pt : poly) {
            const qreal x = pt.x();
            const qreal y = pt.y();
            pt           = { x * kCos - y * kSin, x * kSin + y * kCos };
        }
        m_predefined_shapes.append(radialNormalize(poly));
    }

    Q_ASSERT(m_predefined_shapes.size() == SHAPE_COUNT);

    // Pre-align each target shape to its predecessor so runtime interpolation
    // is a plain LERP with no search overhead.
    m_aligned_shapes.reserve(SHAPE_COUNT);
    for (int i = 0; i < SHAPE_COUNT; ++i) {
        m_aligned_shapes.append(
            alignTo(m_predefined_shapes[i], m_predefined_shapes[(i + 1) % SHAPE_COUNT]));
    }
}

// ── Animated outputs ──────────────────────────────────────────────────────────
auto LoadingIndicatorUpdator::currentShape() const noexcept -> QPolygonF {
    const int    idx  = static_cast<int>(std::floor(m_morph_fraction)) % SHAPE_COUNT;
    const double rawT = m_morph_fraction - std::floor(m_morph_fraction); // [0, 1)
    return interpolateShapes(m_predefined_shapes[idx], m_aligned_shapes[idx], springValue(rawT));
}

auto LoadingIndicatorUpdator::rotation() const noexcept -> double { return m_rotation; }
auto LoadingIndicatorUpdator::color() const noexcept -> QColor { return m_color; }
auto LoadingIndicatorUpdator::morphFraction() const noexcept -> double { return m_morph_fraction; }

// ── Driver ────────────────────────────────────────────────────────────────────
auto LoadingIndicatorUpdator::progress() const noexcept -> double { return m_progress; }

void LoadingIndicatorUpdator::setProgress(double p) {
    if (qFuzzyCompare(m_progress + 1.0, p + 1.0)) return;
    m_progress = p;
    updateInternal();
    emit updated();
}

// ── Configuration ─────────────────────────────────────────────────────────────
auto LoadingIndicatorUpdator::colors() const -> QList<QColor> { return m_colors; }

void LoadingIndicatorUpdator::setColors(const QList<QColor>& colors) {
    if (m_colors == colors) return;
    m_colors = colors;
    emit colorsChanged();

    updateInternal();
    emit updated();
}

// ── Internal update ───────────────────────────────────────────────────────────
// Mirrors MDC-Android's updateIndicatorRotation + updateIndicatorShapeAndColor,
// with the live SpringAnimation replaced by the analytic springValue().
void LoadingIndicatorUpdator::updateInternal() noexcept {
    m_morph_fraction = m_progress;

    const double base       = std::floor(m_morph_fraction); // completed shape steps
    const double rawFrac    = m_morph_fraction - base;      // [0, 1)
    const double springFrac = springValue(rawFrac);         // spring-eased

    // MDC rotation formula:
    //   constantComponent = CONSTANT_ROTATION * (base + rawFrac)
    //   springComponent   = EXTRA_ROTATION    * springFrac
    m_rotation = std::fmod(CONSTANT_ROTATION_PER_SHAPE * (base + rawFrac) +
                               EXTRA_ROTATION_PER_SHAPE * springFrac,
                           360.0);

    // Color — linear interpolation in sRGB, spring-eased, channels clamped
    if (! m_colors.isEmpty()) {
        const int    sz   = m_colors.size();
        const int    idx1 = static_cast<int>(base) % sz;
        const int    idx2 = (idx1 + 1) % sz;
        const double t    = qBound(0.0, springFrac, 1.0);
        const double inv  = 1.0 - t;

        const QColor& c1 = m_colors[idx1];
        const QColor& c2 = m_colors[idx2];
        m_color          = QColor::fromRgbF(qBound(0.0, c1.redF() * inv + c2.redF() * t, 1.0),
                                   qBound(0.0, c1.greenF() * inv + c2.greenF() * t, 1.0),
                                   qBound(0.0, c1.blueF() * inv + c2.blueF() * t, 1.0),
                                   qBound(0.0, c1.alphaF() * inv + c2.alphaF() * t, 1.0));
    }
}

// ── Shape interpolation ───────────────────────────────────────────────────────
// Pure LERP — winding alignment is resolved at construction time.
auto LoadingIndicatorUpdator::interpolateShapes(const QPolygonF& s1, const QPolygonF& s2,
                                                double t) const -> QPolygonF {
    const int n = std::min(s1.size(), s2.size());
    if (n == 0) return {};

    const double inv = 1.0 - t;
    QPolygonF    res;
    res.reserve(n);
    for (int i = 0; i < n; ++i) res.append(s1[i] * inv + s2[i] * t);
    return res;
}

} // namespace qml_material

#include "qml_material/anim/moc_loading_indicator_updator.cpp"
