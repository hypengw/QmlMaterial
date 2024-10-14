#pragma once
#include <QSGGeometry>
#include <QColor>
#include <QVector4D>

#include "qml_material/core.h"

namespace qml_material::sg
{

struct RectangleVertex {
    // position
    float x;
    float y;

    // color
    float r;
    float g;
    float b;
    float a;

    // circle edge
    float ce_x; // if in x edge
    float ce_y; // if in y edge
    float ce_distance_to_outter; // from inner rect
    float ce_distance_to_inner;  // from inner rect

    operator QVector2D() const { return { x, y }; }
    operator QColor() const { return QColor::fromRgbF(r, g, b, a); }
};
auto create_rectangle_geomery() -> up<QSGGeometry>;
// tl tr bl br
void update_rectangle_geomery(RectangleVertex* vertexs, QVector2D size, QColor color,
                              QVector4D radius);

struct ElevationVertex {
    // position
    float x;
    float y;

    // color
    float r;
    float g;
    float b;
    float a;

    // shadow
    float ce_x;
    float ce_y;
    float ce_radius;
};
auto create_elevation_geometry() -> up<QSGGeometry>;

} // namespace qml_material::sg