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

// ── Path sampling ─────────────────────────────────────────────────────────────
QPolygonF samplePath(const QPainterPath& path, int n) {
    QPolygonF poly;
    poly.reserve(n);
    for (int i = 0; i < n; ++i) poly.append(path.pointAtPercent(double(i) / n));
    return poly;
}

// ── Shape constructors ────────────────────────────────────────────────────────
QPolygonF buildRegularPolygon(int vertices, qreal radius, qreal cornerRadius, qreal rotationRad,
                              int n) {
    if (vertices < 2) return {};
    QPainterPath   path;
    QList<QPointF> pts;
    pts.reserve(vertices);
    for (int i = 0; i < vertices; ++i) {
        qreal a = rotationRad + (2.0 * M_PI * i / vertices);
        pts.append({ radius * std::cos(a), radius * std::sin(a) });
    }
    for (int i = 0; i < vertices; ++i) {
        const QPointF p0 = pts[(i + vertices - 1) % vertices];
        const QPointF p1 = pts[i];
        const QPointF p2 = pts[(i + 1) % vertices];
        QPointF       v1 = p1 - p0, v2 = p2 - p1;
        qreal         l1 = std::hypot(v1.x(), v1.y()), l2 = std::hypot(v2.x(), v2.y());
        v1 /= l1;
        v2 /= l2;
        qreal   d = std::min({ cornerRadius, l1 * 0.5, l2 * 0.5 });
        QPointF s = p1 - v1 * d, e = p1 + v2 * d;
        i == 0 ? path.moveTo(s) : path.lineTo(s);
        path.quadTo(p1, e);
    }
    path.closeSubpath();
    return samplePath(path, n);
}

QPolygonF buildStarPolygon(int spokes, qreal outerR, qreal innerR, qreal cornerRadius,
                           qreal rotationRad, int n) {
    QPainterPath   path;
    const int      total = spokes * 2;
    QList<QPointF> pts;
    pts.reserve(total);
    for (int i = 0; i < total; ++i) {
        qreal r = (i % 2 == 0) ? outerR : innerR;
        qreal a = rotationRad + (M_PI * i / spokes);
        pts.append({ r * std::cos(a), r * std::sin(a) });
    }
    for (int i = 0; i < total; ++i) {
        const QPointF p0 = pts[(i + total - 1) % total];
        const QPointF p1 = pts[i];
        const QPointF p2 = pts[(i + 1) % total];
        QPointF       v1 = p1 - p0, v2 = p2 - p1;
        qreal         l1 = std::hypot(v1.x(), v1.y()), l2 = std::hypot(v2.x(), v2.y());
        v1 /= l1;
        v2 /= l2;
        qreal   d = std::min({ cornerRadius, l1 * 0.5, l2 * 0.5 });
        QPointF s = p1 - v1 * d, e = p1 + v2 * d;
        i == 0 ? path.moveTo(s) : path.lineTo(s);
        path.quadTo(p1, e);
    }
    path.closeSubpath();
    return samplePath(path, n);
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

    // Shapes approximate the MDC-Android INDETERMINATE_SHAPES sequence,
    // all normalised to a unit radius [-1, 1].
    m_predefined_shapes.append(buildStarPolygon(10, 1.0, 0.72, 0.18, 0.0, N));     // SOFT_BURST
    m_predefined_shapes.append(buildStarPolygon(9, 1.0, 0.82, 0.28, 0.0, N));      // COOKIE_9
    m_predefined_shapes.append(buildRegularPolygon(5, 1.0, 0.25, -M_PI / 2.0, N)); // PENTAGON
    { // PILL — rounded rectangle
        QPainterPath p;
        p.addRoundedRect(-1.0, -0.38, 2.0, 0.76, 0.38, 0.38);
        m_predefined_shapes.append(samplePath(p, N));
    }
    m_predefined_shapes.append(buildStarPolygon(8, 1.0, 0.85, 0.12, 0.0, N)); // SUNNY
    m_predefined_shapes.append(buildStarPolygon(4, 1.0, 0.68, 0.38, 0.0, N)); // COOKIE_4
    {                                                                         // OVAL (circle)
        QPainterPath p;
        p.addEllipse(QPointF(0, 0), 1.0, 1.0);
        m_predefined_shapes.append(samplePath(p, N));
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
