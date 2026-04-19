#include "qml_material/scenegraph/geometry.h"

#include "qml_material/math/shadow_light.hpp"
#include "qml_material/scenegraph/mesh/nine_patch.hpp"
#include "qml_material/scenegraph/skia/skia_shadow.h"

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

void update_rectangle_geometry(RectangleVertex* v, QVector2D size, QRgb color, QVector4D radius) {
    const auto outer = std::max({ radius[0], radius[1], radius[2], radius[3], 2.0f });
    const auto inner = -1.0f / outer;

    mesh::build_nine_patch(
        v, size, radius,
        [&](RectangleVertex* cell, int corner, QVector2D origin, QVector2D csize) {
            const QVector2D lt = origin;
            const QVector2D rt = origin + QVector2D{ csize.x(), 0 };
            const QVector2D lb = origin + QVector2D{ 0, csize.y() };
            const QVector2D rb = origin + csize;
            auto set_v = [&](int i, QVector2D pos) {
                cell[i].set_point(pos);
                cell[i].ce_x                 = 1.0f;
                cell[i].ce_y                 = 1.0f;
                cell[i].ce_distance_to_outter = outer;
                cell[i].ce_distance_to_inner  = inner;
                cell[i].set_color(color);
            };
            set_v(0, lt);
            set_v(1, rt);
            set_v(2, lb);
            set_v(3, lb);
            set_v(4, rb);
            set_v(5, rt);

            // Pre-refactor convention: the corner's true rrect corner slot is
            // slot 4 when corner_x == 1, else slot 0; same for y.
            const int cx = (corner == 1 || corner == 3) ? 1 : 0;
            const int cy = (corner == 2 || corner == 3) ? 1 : 0;
            cell[cx == 1 ? 4 : 0].ce_x = 1;
            cell[cy == 1 ? 4 : 0].ce_y = 1;
        });
}

// =============================================================================
// Blur Mask Geometry (9-patch halo for analytic rrect gaussian)
// =============================================================================

auto create_blur_mask_geometry() -> up<QSGGeometry> {
    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::createWithAttributeType(
            0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
        QSGGeometry::Attribute::createWithAttributeType(
            1, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
    };
    static QSGGeometry::AttributeSet attrset = { 2, sizeof(BasicVertex), attrs };

    auto out = std::make_unique<QSGGeometry>(attrset, 6 * 9);
    out->setDrawingMode(QSGGeometry::DrawTriangles);
    return out;
}

void update_blur_mask_geometry(BasicVertex* v, QVector2D rect_size, float sigma, QRgb color,
                               QVector4D radius) {
    const float margin = 3.0f * std::max(sigma, 0.0f);

    // Nine-patch outer size covers rect + halo; corner cells hold the rrect curvature
    // (max(radius, margin)) so both curve and blur tail fit inside.
    const QVector2D outer_size {
        rect_size.x() + 2.0f * margin,
        rect_size.y() + 2.0f * margin,
    };
    const QVector4D corner_size {
        std::max<float>(radius[0], margin),
        std::max<float>(radius[1], margin),
        std::max<float>(radius[2], margin),
        std::max<float>(radius[3], margin),
    };

    mesh::build_nine_patch(
        v, outer_size, corner_size,
        [&](BasicVertex* cell, int /*corner*/, QVector2D origin, QVector2D csize) {
            const QVector2D lt = origin;
            const QVector2D rt = origin + QVector2D { csize.x(), 0.0f };
            const QVector2D lb = origin + QVector2D { 0.0f, csize.y() };
            const QVector2D rb = origin + csize;
            auto set_v = [&](int i, QVector2D pos) {
                // Shift so the inner rect's TL is (0, 0) in vertex-local coords;
                // halo extends into negative space.
                cell[i].set_point(pos.x() - margin, pos.y() - margin);
                cell[i].set_color(color);
            };
            set_v(0, lt);
            set_v(1, rt);
            set_v(2, lb);
            set_v(3, lb);
            set_v(4, rb);
            set_v(5, rt);
        });
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
        math::scalar devSpaceInsetWidth = math::ambient_blur_radius(occluderHeight);
        math::scalar devSpaceAmbientBlur = devSpaceInsetWidth * math::ambient_recip_alpha(occluderHeight);
        QRectF ambientRRect = rect.adjusted(-devSpaceInsetWidth, -devSpaceInsetWidth, devSpaceInsetWidth, devSpaceInsetWidth);
        QVector4D ambientRadius = params.radius + QVector4D(devSpaceInsetWidth, devSpaceInsetWidth, devSpaceInsetWidth, devSpaceInsetWidth);
        for(int i=0; i<4; ++i) ambientRadius[i] = std::min<float>(ambientRadius[i], max_radius + devSpaceInsetWidth);

        ops[0] = sk::ShadowCircularRRectOp(params.ambient_color, ambientRRect, ambientRadius, is_oval, devSpaceAmbientBlur,
                                           transparent ? ambientRRect.width() : devSpaceInsetWidth);
    }

    // 2. Spot Shadow Pass
    if (params.spot_color > 0) {
        math::scalar blur, scale; QVector2D offset;
        if (directional) math::directional_params(occluderHeight, params.light_pos.x(), params.light_pos.y(), params.light_pos.z(), params.light_radius, &blur, &scale, &offset);
        else math::spot_params(occluderHeight, params.light_pos.x(), params.light_pos.y(), params.light_pos.z(), params.light_radius, &blur, &scale, &offset);

        float dw = (scale - 1.0f) * rect.width() / 2.0f, dh = (scale - 1.0f) * rect.height() / 2.0f;
        QRectF spotRRect = rect.translated(offset.x(), offset.y()).adjusted(-dw, -dh, dw, dh);
        QVector4D spotRadius = params.radius * scale;

        // InsetWidth calculation to cover the penumbra
        math::scalar insetWidth = blur;
        if (transparent) insetWidth += spotRRect.width();
        else {
            float maxR = std::max({params.radius.x(), params.radius.y(), params.radius.z(), params.radius.w()});
            math::scalar dr = maxR * scale - maxR;
            QVector2D offUL = QVector2D(spotRRect.left() - rect.left() + dr, spotRRect.top() - rect.top() + dr);
            QVector2D offBR = QVector2D(spotRRect.right() - rect.right() - dr, spotRRect.bottom() - rect.bottom() - dr);
            insetWidth += std::max(blur, (math::scalar)std::sqrt(std::max(offUL.lengthSquared(), offBR.lengthSquared())) + dr);
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
