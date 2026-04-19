#pragma once

#include <QRectF>
#include <QVector4D>

namespace qml_material::sg::mesh
{

/// Per-corner geometry info for an rrect 4-corner iteration.
/// `corner_index` is 0=TL, 1=TR, 2=BL, 3=BR.
/// `outer_x` / `outer_y`: the rect's outer edge coord on that corner.
/// `inner_x` / `inner_y`: `outer` shifted inward by `inset` (i.e. the umbra inset point).
/// `arc_x`   / `arc_y`:   the point where the rounded arc starts on the outer edges
///                        (i.e. `outer` shifted inward by `radius[i]`).
/// `sign_x`  / `sign_y`:  outward-normal sign for that corner (+/- 1).
struct rrect_corner {
    int   corner_index;
    float outer_x, outer_y;
    float inner_x, inner_y;
    float arc_x, arc_y;
    float sign_x, sign_y;
    float radius;
};

/// Iterate the four corners of an rrect with mirror-aware coordinates already resolved,
/// invoking `fn(rrect_corner const&)` for each.
template<class Fn>
void for_each_rrect_corner(const QRectF& bounds, QVector4D radius, float inset, Fn&& fn) {
    const float L = bounds.left();
    const float R = bounds.right();
    const float T = bounds.top();
    const float B = bounds.bottom();

    // Corner layout matches the existing TL/TR/BL/BR convention used throughout.
    const struct {
        int   idx;
        float ox, oy;     // outer rect corner
        float sx, sy;     // outward sign
    } sides[4] = {
        { 0, L, T, -1.0f, -1.0f },
        { 1, R, T, +1.0f, -1.0f },
        { 2, L, B, -1.0f, +1.0f },
        { 3, R, B, +1.0f, +1.0f },
    };

    for (auto const& s : sides) {
        const float r = radius[s.idx];
        rrect_corner c {
            .corner_index = s.idx,
            .outer_x      = s.ox,
            .outer_y      = s.oy,
            .inner_x      = s.ox - s.sx * inset,
            .inner_y      = s.oy - s.sy * inset,
            .arc_x        = s.ox - s.sx * r,
            .arc_y        = s.oy - s.sy * r,
            .sign_x       = s.sx,
            .sign_y       = s.sy,
            .radius       = r,
        };
        fn(c);
    }
}

} // namespace qml_material::sg::mesh
