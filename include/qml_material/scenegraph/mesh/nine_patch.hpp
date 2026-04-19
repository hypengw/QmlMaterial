#pragma once

#include <QVector2D>
#include <QVector4D>

#include "qml_material/scenegraph/mesh/quad.hpp"

namespace qml_material::sg::mesh
{

/**
 * Build a rounded-rect 9-patch mesh.
 *
 * Layout: 54 vertices, 6 per cell, 9 cells:
 *   cell 0..3 = TL/TR/BL/BR corners (filled by `corner_fill`)
 *   cell 4..7 = top/bottom/left/right edge quads (stitched from corner verts)
 *   cell 8    = center quad (stitched from corner verts)
 *
 * Each corner cell holds 6 Vertex slots, ordered:
 *   {0: LT, 1: RT, 2: LB, 3: LB, 4: RB, 5: RT}
 * so a triangle-list draw produces two triangles LT-RT-LB and LB-RB-RT.
 *
 * Edge/center cells are copies of corner vertices (position + attributes),
 * so any per-corner-vertex attribute (color, SDF params, etc.) carries through.
 *
 * @param v          Output buffer, size >= 54 * sizeof(Vertex).
 * @param size       Overall rectangle size.
 * @param radius     (TL, TR, BL, BR) corner radii.
 * @param corner_fill
 *     Called once per corner as `corner_fill(v_cell, corner_index, origin, cell_size)`.
 *     The callee populates 6 slots at `v_cell`.
 *     `origin` is the corner cell's top-left in the rect's local space;
 *     `cell_size` is (radius, radius) for that corner.
 */
template<class Vertex, class CornerFillFn>
void build_nine_patch(Vertex* v, QVector2D size, QVector4D radius, CornerFillFn corner_fill) {
    constexpr int u = 6;

    struct cell {
        QVector2D origin;
        QVector2D size;
    };
    const cell corners[4] = {
        { { 0.0f, 0.0f }, { radius[0], radius[0] } },
        { { size.x() - radius[1], 0.0f }, { radius[1], radius[1] } },
        { { 0.0f, size.y() - radius[2] }, { radius[2], radius[2] } },
        { { size.x() - radius[3], size.y() - radius[3] }, { radius[3], radius[3] } },
    };

    for (int i = 0; i < 4; ++i) {
        corner_fill(v + u * i, i, corners[i].origin, corners[i].size);
    }

    // Edge and center cells: stitched by copying corner vertices.
    // See ASCII diagram in docs; vertex indices hand-verified against the pre-refactor
    // src/scenegraph/geometry.cpp::update_rectangle_geometry.
    quad(v + u * 4, v[5],         v[u + 0],     v[4],         v[u + 2]);     // Top
    quad(v + u * 5, v[u * 2 + 1], v[u * 3 + 0], v[u * 2 + 4], v[u * 3 + 2]); // Bottom
    quad(v + u * 6, v[3],         v[4],         v[u * 2 + 0], v[u * 2 + 1]); // Left
    quad(v + u * 7, v[u + 2],     v[u + 4],     v[u * 3 + 0], v[u * 3 + 1]); // Right
    quad(v + u * 8, v[4],         v[u + 2],     v[u * 2 + 1], v[u * 3 + 0]); // Center
}

} // namespace qml_material::sg::mesh
