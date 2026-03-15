#include "qml_material/scenegraph/geometry.h"
#include "qml_material/scenegraph/skia/skia_shadow.h"
#include "qml_material/util/loggingcategory.hpp"

namespace qml_material::sg
{

// =============================================================================
// Rectangle Geometry (SDF Based)
// =============================================================================

auto create_rectangle_geometry() -> up<QSGGeometry> {
    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
        QSGGeometry::Attribute::createWithAttributeType(1, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
        QSGGeometry::Attribute::createWithAttributeType(2, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
    };
    static QSGGeometry::AttributeSet attrset = { 3, sizeof(RectangleVertex), attrs };

    auto out = std::make_unique<QSGGeometry>(attrset, 6 * 9); // 9-patch mesh
    out->setDrawingMode(QSGGeometry::DrawTriangles);
    return out;
}

namespace {
/**
 * @brief Helper to set a rectangular quad (6 vertices for 2 triangles).
 */
template<typename T>
inline void set_quad(T* v, T* lt, T* rt, T* lb, T* rb) {
    v[0] = *lt; v[1] = *rt; v[2] = *lb;
    v[3] = *lb; v[4] = *rb; v[5] = *rt;
}

/**
 * @brief Helper to set a corner mesh cell.
 */
inline void set_corner_vertices(RectangleVertex* v, QVector2D origin, QVector2D size, float outer, float inner, 
                                float cx, float cy, float edge_cx, float edge_cy) {
    auto set_v = [&](int i, QVector2D pos, float x, float y) {
        v[i].set_point(pos); v[i].ce_x = x; v[i].ce_y = y;
        v[i].ce_distance_to_outter = outer; v[i].ce_distance_to_inner = inner;
    };
    QVector2D lt = origin, rt = origin + QVector2D{size.x(), 0}, lb = origin + QVector2D{0, size.y()}, rb = origin + size;
    
    // Fill the 6 vertices for the corner cell
    set_v(0, lt, edge_cx, edge_cy); set_v(1, rt, edge_cx, edge_cy); set_v(2, lb, edge_cx, edge_cy);
    set_v(3, lb, edge_cx, edge_cy); set_v(4, rb, edge_cx, edge_cy); set_v(5, rt, edge_cx, edge_cy);

    // Apply corner specific weights
    v[cx == 1 ? 4 : 0].ce_x = 1;
    v[cy == 1 ? 4 : 0].ce_y = 1;
}
}

void update_rectangle_geometry(RectangleVertex* v, QVector2D size, QRgb color, QVector4D radius) {
    const auto u = 6;
    auto outer = std::max({ radius[0], radius[1], radius[2], radius[3], 2.0f });
    auto inner = -1.0f / outer;

    // Build 4 corners
    set_corner_vertices(v, {0, 0}, {radius[0], radius[0]}, outer, inner, 0, 0, 1, 1); // TL
    set_corner_vertices(v + u, {size.x() - radius[1], 0}, {radius[1], radius[1]}, outer, inner, 1, 0, 1, 1); // TR
    set_corner_vertices(v + u*2, {0, size.y() - radius[2]}, {radius[2], radius[2]}, outer, inner, 0, 1, 1, 1); // BL
    set_corner_vertices(v + u*3, {size.x() - radius[3], size.y() - radius[3]}, {radius[3], radius[3]}, outer, inner, 1, 1, 1, 1); // BR

    // Connect edges and middle using the 9-patch structure
    set_quad(v + u*4, v + 5, v + u + 0, v + 4, v + u + 2); // Top
    set_quad(v + u*5, v + u*2 + 1, v + u*3 + 0, v + u*2 + 4, v + u*3 + 2); // Bottom
    set_quad(v + u*6, v + 3, v + 4, v + u*2 + 0, v + u*2 + 1); // Left
    set_quad(v + u*7, v + u + 2, v + u + 4, v + u*3 + 0, v + u*3 + 1); // Right
    set_quad(v + u*8, v + 4, v + u + 2, v + u*2 + 1, v + u*3 + 0); // Middle

    for (int i = 0; i < u * 9; i++) v[i].set_color(color);
}

// =============================================================================
// Shadow Geometry (High-Fidelity)
// =============================================================================

auto create_shadow_geometry() -> up<QSGGeometry> {
    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
        QSGGeometry::Attribute::createWithAttributeType(1, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
        QSGGeometry::Attribute::createWithAttributeType(2, 3, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
    };
    static QSGGeometry::AttributeSet attrset = { 3, sizeof(ShadowVertex), attrs };
    auto out = std::make_unique<QSGGeometry>(attrset, 0);
    out->setDrawingMode(QSGGeometry::DrawTriangles);
    return out;
}

void update_shadow_geometry(QSGGeometry* geo, const ShadowParams& params, const QRectF& rect) {
    const float occluderHeight = params.z_plane_params.z();
    const bool  transparent    = (params.flags & ShadowFlags::TransparentOccluder_ShadowFlag);
    const bool  directional    = (params.flags & ShadowFlags::DirectionalLight_ShadowFlag);
    const auto  max_radius     = std::min<float>(rect.width(), rect.height()) / 2.0f;
    
    // Detect if we can use circle optimization
    bool is_oval = qFuzzyCompare((float)params.radius.x(), max_radius) && qFuzzyCompare((float)params.radius.y(), max_radius) &&
                   qFuzzyCompare((float)params.radius.z(), max_radius) && qFuzzyCompare((float)params.radius.w(), max_radius) &&
                   qFuzzyCompare((float)rect.width(), (float)rect.height());

    std::array<std::optional<sk::ShadowCircularRRectOp>, 2> ops;

    // 1. Ambient Shadow Pass
    if (params.ambient_color > 0) {
        sk::scalar devSpaceInsetWidth = sk::AmbientBlurRadius(occluderHeight);
        sk::scalar devSpaceAmbientBlur = devSpaceInsetWidth * sk::AmbientRecipAlpha(occluderHeight);
        QRectF ambientRRect = rect.adjusted(-devSpaceInsetWidth, -devSpaceInsetWidth, devSpaceInsetWidth, devSpaceInsetWidth);
        QVector4D ambientRadius = params.radius + QVector4D(devSpaceInsetWidth, devSpaceInsetWidth, devSpaceInsetWidth, devSpaceInsetWidth);
        for(int i=0; i<4; ++i) ambientRadius[i] = std::min<float>(ambientRadius[i], max_radius + devSpaceInsetWidth);

        ops[0] = sk::ShadowCircularRRectOp(params.ambient_color, ambientRRect, ambientRadius, is_oval, devSpaceAmbientBlur, 
                                           transparent ? ambientRRect.width() : devSpaceInsetWidth);
    }

    // 2. Spot Shadow Pass
    if (params.spot_color > 0) {
        sk::scalar blur, scale; QVector2D offset;
        if (directional) sk::GetDirectionalParams(occluderHeight, params.light_pos.x(), params.light_pos.y(), params.light_pos.z(), params.light_radius, &blur, &scale, &offset);
        else sk::GetSpotParams(occluderHeight, params.light_pos.x(), params.light_pos.y(), params.light_pos.z(), params.light_radius, &blur, &scale, &offset);

        float dw = (scale - 1.0f) * rect.width() / 2.0f, dh = (scale - 1.0f) * rect.height() / 2.0f;
        QRectF spotRRect = rect.translated(offset.x(), offset.y()).adjusted(-dw, -dh, dw, dh);
        QVector4D spotRadius = params.radius * scale;

        // InsetWidth calculation to cover the penumbra
        sk::scalar insetWidth = blur;
        if (transparent) insetWidth += spotRRect.width();
        else {
            float maxR = std::max({params.radius.x(), params.radius.y(), params.radius.z(), params.radius.w()});
            sk::scalar dr = maxR * scale - maxR;
            QVector2D offUL = QVector2D(spotRRect.left() - rect.left() + dr, spotRRect.top() - rect.top() + dr);
            QVector2D offBR = QVector2D(spotRRect.right() - rect.right() - dr, spotRRect.bottom() - rect.bottom() - dr);
            insetWidth += std::max(blur, (sk::scalar)std::sqrt(std::max(offUL.lengthSquared(), offBR.lengthSquared())) + dr);
        }

        spotRRect.adjust(-blur, -blur, blur, blur);
        ops[1] = sk::ShadowCircularRRectOp(params.spot_color, spotRRect, spotRadius + QVector4D(blur, blur, blur, blur), is_oval, 2.0f * blur, insetWidth);
    }

    // Finalize mesh allocation and filling
    int v_count = 0, i_count = 0;
    for (auto& p : ops) if (p) { v_count += p->fVertCount; i_count += p->fIndexCount; }
    geo->allocate(v_count, i_count);

    auto v = static_cast<ShadowVertex*>(geo->vertexData());
    auto idx = (std::uint16_t*)geo->indexData();
    int v_off = 0;
    for (auto& p : ops) if (p) {
        if (p->fGeoData.is_circle) p->fillInCircleVerts(sk::ShadowCircularRRectOp::kStroke_RRectType == p->fGeoData.type, &v);
        else p->fillInRRectVerts(&v);
        for (int i = 0; i < p->fIndexCount; i++) idx[i] = p->fIndexPtr[i] + v_off;
        idx += p->fIndexCount; v_off += p->fVertCount;
    }
}

} // namespace qml_material::sg
