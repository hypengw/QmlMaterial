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
 * @brief Shadow rendering implementation based on Skia's ShadowRRectOp.
 * 
 * This module provides the mathematical foundation and mesh generation logic for 
 * high-fidelity shadows. It supports Material Design 3 elevation standards including
 * ambient and spot shadows with independent corner radii.
 */

namespace qml_material::sk
{

using scalar = float;

// =============================================================================
// Math & Conversion Utilities
// =============================================================================

static constexpr scalar SK_Scalar1          = 1.0f;
static constexpr scalar SK_ScalarNearlyZero = (SK_Scalar1 / (1 << 12));

/** Linearly divide and pin the result between min and max. */
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
 * @brief Calculates the blur radius for ambient shadows.
 * @param height The elevation/Z-height of the occluder.
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

/**
 * @class ShadowCircularRRectOp
 * @brief Generates a specialized mesh for a single shadow pass (Ambient or Spot).
 * 
 * This class encapsulates the logic for building a 9-patch-like vertex buffer
 * where the "edges" and "corners" are computed to represent a distance field
 * that creates a smooth shadow falloff in the fragment shader.
 */
class ShadowCircularRRectOp {
public:
    enum RRectType {
        kFill_RRectType,        ///< Standard filled shadow
        kStroke_RRectType,      ///< Shadow for a stroked shape
        kOverstroke_RRectType,  ///< Optimized handling for small/blurry shadows
    };

    /**
     * @brief Raw geometric parameters for the operator.
     */
    struct Geometry {
        QRgb      color;
        QVector4D outer_radius; // TL, TR, BL, BR
        scalar    inner_radius;
        scalar    blur_radius;
        QRectF    dev_bounds;
        RRectType type;
        bool      is_circle;
    };

    /**
     * @brief Constructs the operator and determines the required mesh complexity.
     */
    ShadowCircularRRectOp(QRgb color, const QRectF& devRect, QVector4D devRadius, bool isCircle,
                          float blurRadius, float insetWidth);

    /**
     * @brief Fills the circular (octagon-based) vertex buffer.
     */
    void fillInCircleVerts(bool isStroked, sg::ShadowVertex** vp) const;

    /**
     * @brief Fills the rounded-rectangle vertex buffer.
     * 
     * Handles asymmetrical corners by aligning distance-field offsets with 
     * the actual geometric boundaries to prevent interpolation artifacts.
     */
    void fillInRRectVerts(sg::ShadowVertex** vp) const;

    // Mesh configuration
    Geometry        fGeoData;
    int             fVertCount;
    int             fIndexCount;
    const uint16_t* fIndexPtr;

    // Static helper to map types to index pointers
    static const uint16_t* circle_type_to_indices(bool stroked);
    static const uint16_t* rrect_type_to_indices(RRectType type);
};

} // namespace qml_material::sk
