#include "qml_material/scenegraph/skia/skia_shadow.h"
#include <cmath>
#include <algorithm>

namespace qml_material::sk
{

// =============================================================================
// Parameter Calculations Implementation
// =============================================================================

void GetSpotParams(scalar occluderZ, scalar lightX, scalar lightY, scalar lightZ,
                   scalar lightRadius, scalar* blurRadius, scalar* scale,
                   QVector2D* translate) {
    scalar zRatio = divide_and_pin(occluderZ, lightZ - occluderZ, 0.0f, 0.95f);
    *blurRadius   = lightRadius * zRatio;
    *scale        = divide_and_pin(lightZ, lightZ - occluderZ, 1.0f, 1.95f);
    *translate    = QVector2D(-zRatio * lightX, -zRatio * lightY);
}

void GetDirectionalParams(scalar occluderZ, scalar lightX, scalar lightY, scalar lightZ,
                          scalar lightRadius, scalar* blurRadius, scalar* scale,
                          QVector2D* translate) {
    *blurRadius = lightRadius * occluderZ;
    *scale      = 1;
    // Max z-ratio calculation
    constexpr scalar kMaxZRatio = 64 / SK_ScalarNearlyZero;
    scalar           zRatio     = divide_and_pin(occluderZ, lightZ, 0.0f, kMaxZRatio);
    *translate                  = QVector2D(-zRatio * lightX, -zRatio * lightY);
}

// =============================================================================
// ShadowCircularRRectOp Mesh Index Constants
// =============================================================================

// In the case of a normal fill, we draw geometry for the circle as an octagon.
static constexpr uint16_t gFillCircleIndices[] = {
    0, 1, 8, 1, 2, 8, 2, 3, 8, 3, 4, 8,
    4, 5, 8, 5, 6, 8, 6, 7, 8, 7, 0, 8,
};

// For stroked circles, we use two nested octagons.
static constexpr uint16_t gStrokeCircleIndices[] = {
    0, 1,  9, 0,  9,  8, 1, 2, 10, 1, 10,  9,
    2, 3, 11, 2, 11, 10, 3, 4, 12, 3, 12, 11,
    4, 5, 13, 4, 13, 12, 5, 6, 14, 5, 14, 13,
    6, 7, 15, 6, 15, 14, 7, 0,  8, 7,  8, 15,
};

// Index buffer for rounded rectangles (includes overstroke, corners, edges, and fill)
static constexpr uint16_t gRRectIndices[] = {
    // overstroke quads
    0, 6, 25, 0, 25, 24, 6, 18, 27, 6, 27, 25,
    18, 12, 26, 18, 26, 27, 12, 0, 24, 12, 24, 26,
    // corners
    0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5,
    6, 11, 10, 6, 10, 9, 6, 9, 8, 6, 8, 7,
    12, 17, 16, 12, 16, 15, 12, 15, 14, 12, 14, 13,
    18, 19, 20, 18, 20, 21, 18, 21, 22, 18, 22, 23,
    // edges
    0, 5, 11, 0, 11, 6, 6, 7, 19, 6, 19, 18,
    18, 23, 17, 18, 17, 12, 12, 13, 1, 12, 1, 0,
    // fill quad
    0, 6, 18, 0, 18, 12,
};

static constexpr int kVertsPerStrokeRRect     = 24;
static constexpr int kVertsPerOverstrokeRRect = 28;
static constexpr int kVertsPerFillRRect       = 24;
static constexpr int kVertsPerStrokeCircle   = 16;
static constexpr int kVertsPerFillCircle     = 9;

static constexpr float SK_FloatSqrt2 = 1.41421356f;

// =============================================================================
// ShadowCircularRRectOp Methods
// =============================================================================

const uint16_t* ShadowCircularRRectOp::circle_type_to_indices(bool stroked) {
    return stroked ? gStrokeCircleIndices : gFillCircleIndices;
}

const uint16_t* ShadowCircularRRectOp::rrect_type_to_indices(RRectType type) {
    switch (type) {
    case kFill_RRectType:
    case kStroke_RRectType: return gRRectIndices + 6 * 4;
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
    scalar octOffset = 0.41421356237f;

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

    scalar xOuter[4] = { (float)bounds.left(), (float)bounds.right(), (float)bounds.left(), (float)bounds.right() };
    scalar yOuter[4] = { (float)bounds.top(), (float)bounds.top(), (float)bounds.bottom(), (float)bounds.bottom() };

    auto v = vp[0];
    for (int i = 0; i < 4; ++i) {
        scalar r = args.outer_radius[i];

        scalar xi, xm, yi, ym;
        if (i == 0) { // TL
            xi = bounds.left() + umbra_inset; xm = bounds.left() + r;
            yi = bounds.top() + umbra_inset;  ym = bounds.top() + r;
        } else if (i == 1) { // TR
            xi = bounds.right() - umbra_inset; xm = bounds.right() - r;
            yi = bounds.top() + umbra_inset;   ym = bounds.top() + r;
        } else if (i == 2) { // BL
            xi = bounds.left() + umbra_inset;  xm = bounds.left() + r;
            yi = bounds.bottom() - umbra_inset; ym = bounds.bottom() - r;
        } else { // BR
            xi = bounds.right() - umbra_inset; xm = bounds.right() - r;
            yi = bounds.bottom() - umbra_inset; ym = bounds.bottom() - r;
        }

        // Matching original Skia logic with refined denominator check.
        QVector2D outerVec = QVector2D(r - umbra_inset, -r - umbra_inset).normalized();
        scalar    denom    = SK_FloatSqrt2 * (r - umbra_inset) - r;
        scalar    diagVal  = (std::abs(denom) > SK_ScalarNearlyZero) ? umbra_inset / denom : -1.0f / SK_FloatSqrt2;
        QVector2D diagVec  = QVector2D(diagVal, diagVal);

        auto setV = [&](float x, float y, QVector2D off) {
            v->set_point(x, y); v->set_color(color); v->set_offset(off);
            v->distance_correction = distance_correction; v++;
        };

        setV(xi,        yi,        {  0,  0      });   // 0
        setV(xOuter[i], yi,        {  0, -1      });   // 1
        setV(xOuter[i], ym,        outerVec       );   // 2
        setV(xOuter[i], yOuter[i], diagVec        );   // 3
        setV(xm,        yOuter[i], outerVec       );   // 4
        setV(xi,        yOuter[i], {  0, -1      });   // 5

        // slot order derived directly from index buffer fan direction:
        //   TL: 0,1,2|0,2,3|0,3,4|0,4,5  → slots 1..5 = x-edge→arc→corner→arc→y-edge
        //   TR: 0,5,4|0,4,3|0,3,2|0,2,1  → slots 1..5 = y-edge→arc→corner→arc→x-edge
        //   BL: 0,5,4|0,4,3|0,3,2|0,2,1  → same as TR
        //   BR: 0,1,2|0,2,3|0,3,4|0,4,5  → same as TL
        /*
        setV(xi,        yi,        {  0,  0   });  // slot0: umbra (all corners)
        if (i == 0 || i == 3) {
            // TL / BR: fan goes x-edge → arc-start → corner → arc-end → y-edge
            setV(xOuter[i], yi,        {  0, -1   });  // slot1: x-edge
            setV(xOuter[i], ym,        outerVec    );  // slot2: arc start
            setV(xOuter[i], yOuter[i], diagVec     );  // slot3: corner
            setV(xm,        yOuter[i], outerVec    );  // slot4: arc end
            setV(xi,        yOuter[i], {  0, -1   });  // slot5: y-edge
        } else {
            // TR / BL: fan goes y-edge → arc-end → corner → arc-start → x-edge
            setV(xi,        yOuter[i], {  0, -1   });  // slot1: y-edge
            setV(xm,        yOuter[i], outerVec    );  // slot2: arc end
            setV(xOuter[i], yOuter[i], diagVec     );  // slot3: corner
            setV(xOuter[i], ym,        outerVec    );  // slot4: arc start
            setV(xOuter[i], yi,        {  0, -1   });  // slot5: x-edge
        }
            */
    }

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
