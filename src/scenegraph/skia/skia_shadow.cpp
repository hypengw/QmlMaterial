#include "qml_material/scenegraph/skia/skia_shadow.h"

#include <algorithm>
#include <cmath>

#include "qml_material/math/scalar.hpp"
#include "qml_material/scenegraph/mesh/corner_fan.hpp"

namespace {
using scalar = qml_material::math::scalar;
inline constexpr scalar k_nearly_zero = qml_material::math::k_nearly_zero;
inline constexpr scalar k_float_sqrt2 = qml_material::math::k_float_sqrt2;
} // namespace

namespace qml_material::sk
{

// =============================================================================
// ShadowCircularRRectOp Mesh Index Constants
// =============================================================================

/**
 * Circle mesh indices (Octagon-based).
 * Layout: 8 outer vertices, 1 center vertex.
 */
static constexpr uint16_t gFillCircleIndices[] = {
    0, 1, 8, 1, 2, 8, 2, 3, 8, 3, 4, 8,
    4, 5, 8, 5, 6, 8, 6, 7, 8, 7, 0, 8,
};

/**
 * Stroked circle mesh indices (Two nested octagons).
 */
static constexpr uint16_t gStrokeCircleIndices[] = {
    0, 1,  9, 0,  9,  8, 1, 2, 10, 1, 10,  9,
    2, 3, 11, 2, 11, 10, 3, 4, 12, 3, 12, 11,
    4, 5, 13, 4, 13, 12, 5, 6, 14, 5, 14, 13,
    6, 7, 15, 6, 15, 14, 7, 0,  8, 7,  8, 15,
};

/**
 * RRect Index Buffer (The 9-patch shadow engine).
 * 
 * ASCII Visualization of the 9-patch structure:
 * (Numbers represent the starting vertex of each corner's 6-vertex block)
 * 
 *    0 --- (E) --- 6
 *    |             |
 *   (E)   [Quad]  (E)
 *    |             |
 *    12--- (E) --- 18
 */
static constexpr uint16_t gRRectIndices[] = {
    // --- Overstroke quads (Optional inner ring) ---
    0, 6, 25, 0, 25, 24, 6, 18, 27, 6, 27, 25,
    18, 12, 26, 18, 26, 27, 12, 0, 24, 12, 24, 26,
    
    // --- Corners (Each corner is a 4-triangle fan) ---
    // TL (0..5)
    0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5,
    // TR (6..11) - Note: Static indices handle reverse winding
    6, 11, 10, 6, 10, 9, 6, 9, 8, 6, 8, 7,
    // BL (12..17) - Reverse winding in indices
    12, 17, 16, 12, 16, 15, 12, 15, 14, 12, 14, 13,
    // BR (18..23) - Forward winding
    18, 19, 20, 18, 20, 21, 18, 21, 22, 18, 22, 23,
    
    // --- Edge quads (Connecting the corners) ---
    0, 5, 11, 0, 11, 6, 6, 7, 19, 6, 19, 18,
    18, 23, 17, 18, 17, 12, 12, 13, 1, 12, 1, 0,
    
    // --- Center fill quad ---
    0, 6, 18, 0, 18, 12,
};

static constexpr int kVertsPerStrokeRRect     = 24;
static constexpr int kVertsPerOverstrokeRRect = 28;
static constexpr int kVertsPerFillRRect       = 24;
static constexpr int kVertsPerStrokeCircle   = 16;
static constexpr int kVertsPerFillCircle     = 9;


// =============================================================================
// ShadowCircularRRectOp Methods
// =============================================================================

const uint16_t* ShadowCircularRRectOp::circle_type_to_indices(bool stroked) {
    return stroked ? gStrokeCircleIndices : gFillCircleIndices;
}

const uint16_t* ShadowCircularRRectOp::rrect_type_to_indices(RRectType type) {
    switch (type) {
    case kFill_RRectType:
    case kStroke_RRectType: return gRRectIndices + 6 * 4; // Skip overstroke quads
    case kOverstroke_RRectType: return gRRectIndices;
    }
    return nullptr;
}

ShadowCircularRRectOp::ShadowCircularRRectOp(QRgb color, const QRectF& devRect, QVector4D devRadius, bool isCircle,
                                             float blurRadius, float insetWidth)
    : fIndexPtr(nullptr) {
    QRectF bounds = devRect;
    scalar innerRadius = 0.0f;
    
    RRectType type = kFill_RRectType;
    if (isCircle) {
        innerRadius = devRadius.x() - insetWidth;
        type        = innerRadius > 0 ? kStroke_RRectType : kFill_RRectType;
    } else {
        if (insetWidth <= 0.5f * std::min(devRect.width(), devRect.height())) {
            float maxRadius = std::max({devRadius.x(), devRadius.y(), devRadius.z(), devRadius.w()});
            float umbraInsetEstimate = std::max(maxRadius, blurRadius);
            innerRadius = std::max(insetWidth - umbraInsetEstimate, 0.0f);
            type        = innerRadius > 0 ? kOverstroke_RRectType : kStroke_RRectType;
        }
    }

    fGeoData = (Geometry { color, devRadius, innerRadius, blurRadius, bounds, type, isCircle });
    
    if (isCircle) {
        fVertCount  = (kStroke_RRectType == type) ? kVertsPerStrokeCircle : kVertsPerFillCircle;
        fIndexCount = (kStroke_RRectType == type) ? std::size(gStrokeCircleIndices) : std::size(gFillCircleIndices);
        fIndexPtr   = circle_type_to_indices(kStroke_RRectType == type);
    } else {
        switch (type) {
        case kFill_RRectType: fVertCount = kVertsPerFillRRect; fIndexCount = std::size(gRRectIndices) - 6 * 4; break;
        case kStroke_RRectType: fVertCount = kVertsPerStrokeRRect; fIndexCount = std::size(gRRectIndices) - 6 * 4 - 6; break;
        case kOverstroke_RRectType: fVertCount = kVertsPerOverstrokeRRect; fIndexCount = std::size(gRRectIndices) - 6; break;
        }
        fIndexPtr = rrect_type_to_indices(type);
    }
}

void ShadowCircularRRectOp::fillInCircleVerts(bool isStroked, sg::ShadowVertex** vp) const {
    const auto& args               = this->fGeoData;
    QRgb        color              = args.color;
    scalar      outerRadius        = args.outer_radius.x();
    scalar      innerRadius        = args.inner_radius / outerRadius;
    scalar      blurRadius         = args.blur_radius;
    scalar      distanceCorrection = outerRadius / blurRadius;

    const QRectF& bounds = args.dev_bounds;
    auto   center    = QVector2D(bounds.center().x(), bounds.center().y());
    scalar halfWidth = 0.5f * bounds.width();
    scalar octOffset = 0.41421356237f; // tan(pi/8)

    auto v = vp[0];
    QVector2D offsets[8] = {
        {-octOffset, -1.0f}, {octOffset, -1.0f}, {1.0f, -octOffset}, {1.0f, octOffset},
        {octOffset, 1.0f}, {-octOffset, 1.0f}, {-1.0f, octOffset}, {-1.0f, -octOffset}
    };

    for (int i = 0; i < 8; ++i) {
        v->set_point(center + offsets[i] * halfWidth);
        v->set_color(color);
        v->set_offset(offsets[i]);
        v->distance_correction = distanceCorrection;
        v++;
    }

    if (isStroked) {
        scalar c = 0.923579533f, s = 0.382683432f, r = args.inner_radius;
        QVector2D iOffsets[8] = { {-s, -c}, {s, -c}, {c, -s}, {c, s}, {s, c}, {-s, c}, {-c, s}, {-c, -s} };
        for (int i = 0; i < 8; ++i) {
            v->set_point(center + iOffsets[i] * r);
            v->set_color(color);
            v->set_offset(iOffsets[i] * innerRadius);
            v->distance_correction = distanceCorrection;
            v++;
        }
    } else {
        v->set_point(center); v->set_color(color); v->set_offset({ 0, 0 });
        v->distance_correction = distanceCorrection; v++;
    }
    vp[0] = v;
}

/**
 * Fills the vertex buffer for a rounded rectangle shadow.
 * 
 * ASCII Visualization of a Corner Fan:
 * 
 * (Slot 5) -- [Slot 0] -- (Slot 1)  <-- Slot 0 is the Umbra point (offset 0,0)
 *    |         /  |  \       |
 *    |       /    |    \     |      Slots 1-5 are on the outer edge.
 * (Slot 4) -- [Slot 3] -- (Slot 2)  <-- Slot 3 is the true geometric corner.
 */
void ShadowCircularRRectOp::fillInRRectVerts(sg::ShadowVertex** vp) const {
    const Geometry& args         = this->fGeoData;
    QRgb            color        = args.color;
    const QRectF&   bounds       = args.dev_bounds;
    scalar          blurRadius   = args.blur_radius;

    scalar min_dim = 0.5f * std::min(bounds.width(), bounds.height());

    // Consistent umbra_inset across all corners to maintain rectangular inner geometry.
    float  maxR                = std::max({ args.outer_radius.x(), args.outer_radius.y(), args.outer_radius.z(), args.outer_radius.w() });
    scalar umbra_inset         = std::min(std::max(maxR, blurRadius), min_dim);
    scalar distance_correction = umbra_inset / blurRadius;

    auto v = vp[0];
    sg::mesh::for_each_rrect_corner(bounds, args.outer_radius, umbra_inset, [&](const sg::mesh::rrect_corner& c) {
        const scalar r = c.radius;

        // Distance Field Offset Math (Normalized to unified abs() system in shader)
        QVector2D outerVec = QVector2D(r - umbra_inset, -r - umbra_inset).normalized();
        scalar    denom    = k_float_sqrt2 * (r - umbra_inset) - r;
        scalar    diagVal  = (std::abs(denom) > k_nearly_zero) ? umbra_inset / denom : -1.0f / k_float_sqrt2;
        QVector2D diagVec  = QVector2D(diagVal, diagVal);

        auto setV = [&](float x, float y, QVector2D off) {
            v->set_point(x, y); v->set_color(color); v->set_offset(off);
            v->distance_correction = distance_correction; v++;
        };

        // All corners use the same local vertex fill order (0..5).
        // The static index buffer (gRRectIndices) is already pre-configured to
        // handle the symmetry and correct winding for TR, BL, and BR corners
        // by picking these vertices in the appropriate order.
        setV(c.inner_x, c.inner_y, {  0,  0 });  // 0: umbra
        setV(c.outer_x, c.inner_y, {  0, -1 });  // 1: x-axis edge
        setV(c.outer_x, c.arc_y,   outerVec );   // 2: arc start
        setV(c.outer_x, c.outer_y, diagVec  );   // 3: corner
        setV(c.arc_x,   c.outer_y, outerVec );   // 4: arc end
        setV(c.inner_x, c.outer_y, {  0, -1 });  // 5: y-axis edge
    });

    if (kOverstroke_RRectType == args.type) {
        scalar inset = umbra_inset + args.inner_radius;
        scalar px[4] = {(float)bounds.left() + inset, (float)bounds.right() - inset,
                        (float)bounds.left() + inset, (float)bounds.right() - inset};
        scalar py[4] = {(float)bounds.top()  + inset, (float)bounds.top()   + inset,
                        (float)bounds.bottom()-inset,  (float)bounds.bottom()-inset};
        for (int i = 0; i < 4; ++i) {
            v->set_point(px[i], py[i]); v->set_color(color); v->set_offset({0, 0});
            v->distance_correction = distance_correction; v++;
        }
    }
    vp[0] = v;
}

} // namespace qml_material::sk
