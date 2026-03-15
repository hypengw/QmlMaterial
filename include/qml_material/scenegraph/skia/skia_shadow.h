#pragma once

#include <QRectF>
#include <QVector2D>
#include <QVector4D>
#include <QColor>
#include <optional>
#include <array>

#include "qml_material/scenegraph/geometry.h"

/**
 * @file skia_shadow.h
 * @brief High-fidelity shadow rendering based on Skia's analytic shadow model.
 * 
 * ### Architecture Overview
 * This module implements a specialized mesh generator for Material Design 3 shadows.
 * Instead of using Gaussian blur (which is expensive and hard to clip), we use:
 * 1. **Analytic Geometry**: A 9-patch mesh where corners are vertex fans.
 * 2. **Distance Fields**: Vertices carry a 2D "offset" representing the normalized 
 *    distance to the nearest edge/corner.
 * 3. **LUT-based Falloff**: The fragment shader uses these offsets to sample a 
 *    1D strength texture (fade-off LUT) to create smooth, physically-accurate penumbras.
 * 
 * ### Mesh Structure (Rounded Rectangle)
 * The mesh consists of 4 corners (6 vertices each) and an optional inner quad for 
 * overstroked shadows.
 */

namespace qml_material::sk
{

using scalar = float;

// =============================================================================
// Math & Conversion Utilities
// =============================================================================

static constexpr scalar SK_Scalar1          = 1.0f;
static constexpr scalar SK_ScalarNearlyZero = (SK_Scalar1 / (1 << 12));

/** 
 * @brief Linearly divide and pin the result between min and max. 
 * Prevents NaN and ensures values stay within valid sampling ranges.
 */
static inline float divide_and_pin(float numer, float denom, float min, float max) {
    float result = std::max(min, std::min(numer / denom, max));
    return result;
}

template<typename T>
static constexpr const T& SkTPin(const T& x, const T& lo, const T& hi) {
    return std::max(lo, std::min(x, hi));
}

constexpr auto sk_float_round(double x) { return (float)std::floor((x) + 0.5); }

/**
 * @brief Safely converts a float to a saturated 32-bit integer.
 */
static constexpr int sk_float_saturate2int(float x) {
    constexpr int SK_MaxS32FitsInFloat = 2147483520;
    constexpr int SK_MinS32FitsInFloat = -SK_MaxS32FitsInFloat;
    x = x < SK_MaxS32FitsInFloat ? x : (float)SK_MaxS32FitsInFloat;
    x = x > SK_MinS32FitsInFloat ? x : (float)SK_MinS32FitsInFloat;
    return (int)x;
}

constexpr auto SkScalarRoundToInt(double x) { return sk_float_saturate2int(sk_float_round(x)); }

// =============================================================================
// Material 3 Shadow Parameter Calculators
// =============================================================================

static constexpr auto kAmbientHeightFactor = 1.0f / 128.0f;
static constexpr auto kAmbientGeomFactor   = 64.0f;
static constexpr auto kMaxAmbientRadius    = 300 * kAmbientHeightFactor * kAmbientGeomFactor;

/**
 * @brief Calculates the blur radius for ambient shadows based on Z-height.
 */
inline scalar AmbientBlurRadius(scalar height) {
    return std::min(height * kAmbientHeightFactor * kAmbientGeomFactor, kMaxAmbientRadius);
}

/**
 * @brief Calculates the reciprocal alpha for ambient shadows to adjust intensity.
 */
inline scalar AmbientRecipAlpha(scalar height) {
    return 1.0f + std::max(height * kAmbientHeightFactor, 0.0f);
}

/**
 * @brief Calculates spot shadow parameters based on a virtual point light source.
 */
void GetSpotParams(scalar occluderZ, scalar lightX, scalar lightY, scalar lightZ,
                   scalar lightRadius, scalar* blurRadius, scalar* scale,
                   QVector2D* translate);

/**
 * @brief Calculates directional shadow parameters (simplified spot model).
 */
void GetDirectionalParams(scalar occluderZ, scalar lightX, scalar lightY, scalar lightZ,
                          scalar lightRadius, scalar* blurRadius, scalar* scale,
                          QVector2D* translate);

// =============================================================================
// Shadow Geometry Operator
// =============================================================================

class ShadowCircularRRectOp {
public:
    enum RRectType {
        kFill_RRectType,        ///< Normal shadow filling the entire interior.
        kStroke_RRectType,      ///< Hollow shadow for outlined shapes.
        kOverstroke_RRectType,  ///< Optimized for large blurs where the umbra is tiny.
    };

    struct Geometry {
        QRgb      color;
        QVector4D outer_radius; // TL, TR, BL, BR
        scalar    inner_radius;
        scalar    blur_radius;
        QRectF    dev_bounds;
        RRectType type;
        bool      is_circle;
    };

    ShadowCircularRRectOp(QRgb color, const QRectF& devRect, QVector4D devRadius, bool isCircle,
                          float blurRadius, float insetWidth);

    void fillInCircleVerts(bool isStroked, sg::ShadowVertex** vp) const;
    void fillInRRectVerts(sg::ShadowVertex** vp) const;

    Geometry        fGeoData;
    int             fVertCount;
    int             fIndexCount;
    const uint16_t* fIndexPtr;

    static const uint16_t* circle_type_to_indices(bool stroked);
    static const uint16_t* rrect_type_to_indices(RRectType type);
};

} // namespace qml_material::sk
