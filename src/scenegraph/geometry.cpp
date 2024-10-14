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

inline void set_rect(RectangleVertex* v, RectangleVertex* lt, RectangleVertex* rt,
                     RectangleVertex* lb, RectangleVertex* rb) {
    v[0] = *lt;
    v[1] = *rt;
    v[2] = *lb;
    v[3] = *lb;
    v[4] = *rb;
    v[5] = *rt;
}

// four points

inline void set_corner(RectangleVertex* v, QVector2D o, QVector2D s, float outter, float inner) {
    auto helper = [](RectangleVertex* v,
                     QVector2D        lt,
                     QVector2D        rt,
                     QVector2D        lb,
                     QVector2D        rb,
                     float            outter,
                     float            inner) {
        v[0].x                     = lt.x();
        v[0].y                     = lt.y();
        v[0].ce_x                  = 0;
        v[0].ce_y                  = 0;
        v[0].ce_distance_to_outter = outter;
        v[0].ce_distance_to_inner  = inner;

        v[1].x                     = rt.x();
        v[1].y                     = rt.y();
        v[1].ce_x                  = 0;
        v[1].ce_y                  = 0;
        v[1].ce_distance_to_outter = outter;
        v[1].ce_distance_to_inner  = inner;

        v[2].x                     = lb.x();
        v[2].y                     = lb.y();
        v[2].ce_x                  = 0;
        v[2].ce_y                  = 0;
        v[2].ce_distance_to_outter = outter;
        v[2].ce_distance_to_inner  = inner;

        v[3].x                     = lb.x();
        v[3].y                     = lb.y();
        v[3].ce_x                  = 0;
        v[3].ce_y                  = 0;
        v[3].ce_distance_to_outter = outter;
        v[3].ce_distance_to_inner  = inner;

        v[4].x                     = rb.x();
        v[4].y                     = rb.y();
        v[4].ce_x                  = 0;
        v[4].ce_y                  = 0;
        v[4].ce_distance_to_outter = outter;
        v[4].ce_distance_to_inner  = inner;

        v[5].x                     = rt.x();
        v[5].y                     = rt.y();
        v[5].ce_x                  = 0;
        v[5].ce_y                  = 0;
        v[5].ce_distance_to_outter = outter;
        v[5].ce_distance_to_inner  = inner;
    };
    QVector2D lt = o;
    QVector2D rt = o + QVector2D { s.x(), 0 };
    QVector2D lb = o + QVector2D { 0, s.y() };
    QVector2D rb = o + s;
    helper(v, lt, rt, lb, rb, outter, inner);
};
inline void set_right_bottom_corner(RectangleVertex* v, QVector2D size, float r, float outter,
                                    float inner) {
    set_corner(v, { size.x() - r, size.y() - r }, { r, r }, outter, inner);
    v[1].ce_x = 1;
    v[5].ce_x = 1;

    v[2].ce_y = 1;
    v[3].ce_y = 1;

    v[4].ce_x = 1;
    v[4].ce_y = 1;
}
inline void set_right_top_corner(RectangleVertex* v, QVector2D size, float r, float outter,
                                 float inner) {
    set_corner(v, { size.x() - r, 0 }, { r, r }, outter, inner);
    v[4].ce_x = 1;
    v[0].ce_y = 1;

    v[1].ce_x = 1;
    v[1].ce_y = 1;
    v[5].ce_x = 1;
    v[5].ce_y = 1;
}
inline void set_left_bottom_corner(RectangleVertex* v, QVector2D size, float r, float outter,
                                   float inner) {
    set_corner(v, { 0, size.y() - r }, { r, r }, outter, inner);
    v[0].ce_x = 1;
    v[4].ce_y = 1;

    v[2].ce_x = 1;
    v[2].ce_y = 1;
    v[3].ce_x = 1;
    v[3].ce_y = 1;
}
inline void set_left_top_corner(RectangleVertex* v, QVector2D, float r, float outter, float inner) {
    set_corner(v, {}, { r, r }, outter, inner);
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

    auto outter = std::max({ radius[0], radius[1], radius[2], radius[3], 2.0f });
    auto inner  = -1.0f / outter;

    set_left_top_corner(lt, size, radius[0], outter, inner);
    set_right_top_corner(rt, size, radius[1], outter, inner);
    set_left_bottom_corner(lb, size, radius[2], outter, inner);
    set_right_bottom_corner(rb, size, radius[3], outter, inner);

    auto top    = v + u * 4;
    auto left   = v + u * 5;
    auto bottom = v + u * 6;
    auto right  = v + u * 7;
    auto middle = v + u * 8;

    set_rect(top, lt + 5, rt + 0, lt + 4, rt + 2);
    set_rect(left, lt + 3, lt + 4, lb + 0, lb + 1);
    set_rect(bottom, lb + 1, rb + 0, lb + 4, rb + 2);
    set_rect(right, rt + 2, rt + 4, rb + 0, rb + 1);
    set_rect(middle, lt + 4, rt + 2, lb + 1, rb + 0);

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