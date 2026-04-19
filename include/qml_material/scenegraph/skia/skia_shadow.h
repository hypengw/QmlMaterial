#pragma once

#include <QRectF>
#include <QVector4D>

#include "qml_material/scenegraph/geometry.h"

/**
 * @file skia_shadow.h
 * @brief 9-patch mesh + distance-field rrect/circle shadow operator.
 *
 * Math utilities (sigma, ambient/spot parameter calculators) live in
 * `qml_material::math`. This file only holds the mesh-generating operator.
 */

namespace qml_material::sk
{

using scalar = float;

class ShadowCircularRRectOp {
public:
    enum RRectType {
        kFill_RRectType,       ///< Normal shadow filling the entire interior.
        kStroke_RRectType,     ///< Hollow shadow for outlined shapes.
        kOverstroke_RRectType, ///< Optimized for large blurs where the umbra is tiny.
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
