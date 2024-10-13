#include "qml_material/scenegraph/geometry.h"

namespace qml_material
{

namespace sg
{
namespace
{
/*
  0 ------- 1
   |      /
   |     /
   |    /
   |   /
   |  /
   | /
  2
            5
          / |
         /  |
        /   |
       /    |
      /     |
     /      |
  3 ------- 4
*/

inline auto set_color(RectangleVertex* v, const QColor& r) {
    v->r = r.redF();
    v->g = r.greenF();
    v->b = r.blueF();
    v->a = r.alphaF();
}

// four points
inline void set_rect(RectangleVertex* v, QVector2D lt, QVector2D rt, QVector2D lb, QVector2D rb,
                     float r = 1.0f) {
    v[0].x         = lt.x();
    v[0].y         = lt.y();
    v[0].ce_x      = 0;
    v[0].ce_y      = 0;
    v[0].ce_radius = r;

    v[1].x         = rt.x();
    v[1].y         = rt.y();
    v[1].ce_x      = 0;
    v[1].ce_y      = 0;
    v[1].ce_radius = r;

    v[2].x         = lb.x();
    v[2].y         = lb.y();
    v[2].ce_x      = 0;
    v[2].ce_y      = 0;
    v[2].ce_radius = r;

    v[3].x         = lb.x();
    v[3].y         = lb.y();
    v[3].ce_x      = 0;
    v[3].ce_y      = 0;
    v[3].ce_radius = r;

    v[4].x         = rb.x();
    v[4].y         = rb.y();
    v[4].ce_x      = 0;
    v[4].ce_y      = 0;
    v[4].ce_radius = r;

    v[5].x         = rt.x();
    v[5].y         = rt.y();
    v[5].ce_x      = 0;
    v[5].ce_y      = 0;
    v[5].ce_radius = r;
}
inline void set_corner(RectangleVertex* v, QVector2D o, QVector2D s, float r) {
    QVector2D lt = o;
    QVector2D rt = o + QVector2D { s.x(), 0 };
    QVector2D lb = o + QVector2D { 0, s.y() };
    QVector2D rb = o + s;
    set_rect(v, lt, rt, lb, rb, r);
};
inline void set_right_bottom_corner(RectangleVertex* v, QVector2D size, float r) {
    set_corner(v, { size.x() - r, size.y() - r }, { r, r }, r);
    v[1].ce_x = 1;
    v[5].ce_x = 1;

    v[2].ce_y = 1;
    v[3].ce_y = 1;

    v[4].ce_x = 1;
    v[4].ce_y = 1;
}
inline void set_right_top_corner(RectangleVertex* v, QVector2D size, float r) {
    set_corner(v, { size.x() - r, 0 }, { r, r }, r);
    v[4].ce_x = 1;
    v[0].ce_y = 1;

    v[1].ce_x = 1;
    v[1].ce_y = 1;
    v[5].ce_x = 1;
    v[5].ce_y = 1;
}
inline void set_left_bottom_corner(RectangleVertex* v, QVector2D size, float r) {
    set_corner(v, { 0, size.y() - r }, { r, r }, r);
    v[0].ce_x = 1;
    v[4].ce_y = 1;

    v[2].ce_x = 1;
    v[2].ce_y = 1;
    v[3].ce_x = 1;
    v[3].ce_y = 1;
}
inline void set_left_top_corner(RectangleVertex* v, QVector2D, float r) {
    set_corner(v, {}, { r, r }, r);
    v[2].ce_x = 1;
    v[3].ce_x = 1;

    v[1].ce_y = 1;
    v[5].ce_y = 1;

    v[0].ce_x = 1;
    v[0].ce_y = 1;
}

} // namespace
} // namespace sg

auto sg::create_rectangle_geomery() -> up<QSGGeometry> {
    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::createWithAttributeType(
            0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
        QSGGeometry::Attribute::createWithAttributeType(
            1, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
        QSGGeometry::Attribute::createWithAttributeType(
            2, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
    };

    static QSGGeometry::AttributeSet attrset = { 3, sizeof(RectangleVertex), attrs };

    auto out = std::make_unique<QSGGeometry>(attrset, 6 * 9);
    out->setDrawingMode(QSGGeometry::DrawTriangles);
    return out;
}

void sg::update_rectangle_geomery(RectangleVertex* v, QVector2D size, QColor color,
                                  QVector4D radius) {
    const auto u = 6;

    auto lt = v;
    auto rt = v + u;
    auto lb = v + u * 2;
    auto rb = v + u * 3;

    set_left_top_corner(lt, size, radius[0]);
    set_right_top_corner(rt, size, radius[1]);
    set_left_bottom_corner(lb, size, radius[2]);
    set_right_bottom_corner(rb, size, radius[3]);

    auto top    = v + u * 4;
    auto left   = v + u * 5;
    auto bottom = v + u * 6;
    auto right  = v + u * 7;
    auto middle = v + u * 8;

    set_rect(top, lt[5], rt[0], lt[4], rt[2]);
    set_rect(left, lt[3], lt[4], lb[0], lb[1]);
    set_rect(bottom, lb[1], rb[0], lb[4], rb[2]);
    set_rect(right, rt[2], rt[4], rb[0], rb[1]);
    set_rect(middle, lt[4], rt[2], lb[1], rb[0]);

    for (int i = 0; i < u * 9; i++) {
        set_color(v + i, color);
    }
}

auto sg::create_elevation_geometry() -> up<QSGGeometry> {
    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::createWithAttributeType(
            0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
        QSGGeometry::Attribute::createWithAttributeType(
            1, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
        QSGGeometry::Attribute::createWithAttributeType(
            2, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
    };

    static QSGGeometry::AttributeSet attrset = { 3, sizeof(ElevationVertex), attrs };

    auto out = std::make_unique<QSGGeometry>(attrset, 6 * 9);
    out->setDrawingMode(QSGGeometry::DrawTriangles);
    return out;
}

} // namespace qml_material