#include "qml_material/scenegraph/blur_mask_material.h"

#include <cstring>

#include <QSGTexture>

#include "qml_material/scenegraph/texture_cache.h"

namespace qml_material::sg
{

namespace
{
constexpr auto kVertPath = ":/Qcm/Material/assets/shader/blur_mask.vert.qsb";
constexpr auto kFragPath = ":/Qcm/Material/assets/shader/blur_mask.frag.qsb";
} // namespace

BlurMaskMaterial::BlurMaskMaterial() {
    setFlag(QSGMaterial::Blending, true);
    // Prevent Qt's batch renderer from flattening the item transform into
    // vertex data — we need `in_position` to stay item-local in the shader
    // so coverage_1d uses the correct coords against rect_size.
    setFlag(QSGMaterial::RequiresFullMatrix, true);
}

float BlurMaskMaterial::effective_radius() const {
    return std::max({ radius.x(), radius.y(), radius.z(), radius.w() });
}

QSGMaterialShader* BlurMaskMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new BlurMaskShader {};
}

QSGMaterialType* BlurMaskMaterial::type() const {
    static QSGMaterialType staticType;
    return &staticType;
}

int BlurMaskMaterial::compare(const QSGMaterial* other) const {
    auto* o = static_cast<const BlurMaskMaterial*>(other);
    // Cheap-first ordering: pointers, then scalar uniforms. RequiresFullMatrix
    // already disables vertex-baked batching, but a meaningful compare still
    // helps the renderer keep equivalent draws in the same render-state group
    // and avoid spurious shader/uniform rebinds.
    auto cmp_ptr = [](const void* a, const void* b) {
        return a == b ? 0 : (a < b ? -1 : 1);
    };
    if (int c = cmp_ptr(m_profile_texture, o->m_profile_texture); c != 0) return c;
    for (int i = 0; i < 4; ++i) {
        if (int c = cmp_ptr(m_corner_textures[i].texture, o->m_corner_textures[i].texture); c != 0)
            return c;
    }
    if (style != o->style) return int(style) < int(o->style) ? -1 : 1;
    if (sigma != o->sigma) return sigma < o->sigma ? -1 : 1;
    if (rect_size.x() != o->rect_size.x()) return rect_size.x() < o->rect_size.x() ? -1 : 1;
    if (rect_size.y() != o->rect_size.y()) return rect_size.y() < o->rect_size.y() ? -1 : 1;
    for (int i = 0; i < 4; ++i) {
        if (radius[i] != o->radius[i]) return radius[i] < o->radius[i] ? -1 : 1;
    }
    return 0;
}

auto BlurMaskMaterial::profile_texture() -> QSGTexture* { return m_profile_texture; }
const CornerCutoutTexture& BlurMaskMaterial::corner_texture(int index) const {
    return m_corner_textures[index];
}

void BlurMaskMaterial::init_profile_texture(QQuickWindow* win) {
    m_profile_texture = shared_blur_profile_texture(win);
}

void BlurMaskMaterial::init_corner_textures(QQuickWindow* win) {
    for (int i = 0; i < 4; ++i) {
        m_corner_textures[i] = shared_rrect_corner_cutout_texture(win, sigma, radius[i]);
    }
}

BlurMaskShader::BlurMaskShader() {
    setShaderFileName(QSGMaterialShader::VertexStage, QLatin1String(kVertPath));
    setShaderFileName(QSGMaterialShader::FragmentStage, QLatin1String(kFragPath));
}

bool BlurMaskShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial*) {
    auto*       mat = static_cast<BlurMaskMaterial*>(newMaterial);
    QByteArray* buf = state.uniformData();
    // std140 block:
    //   mat4  qt_Matrix  @ 0   (64)
    //   float qt_Opacity @ 64  (4)
    //   float sigma      @ 68  (4)
    //   vec2  rect_size  @ 72  (8)
    //   int   style      @ 80  (4)
    //   float radius     @ 84  (4)   max of the four
    //   float radius_tl  @ 88  (4)
    //   float radius_tr  @ 92  (4)
    //   float radius_bl  @ 96  (4)
    //   float radius_br  @ 100 (4)
    //   vec4 corner_uv[4] @ 112 (64): margin, extent, unused, unused
    Q_ASSERT(buf->size() >= 176);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
    }
    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
    }

    // The same material can be mutated between frames during corner animations.
    // The rectangle and its cutouts must use the same quantised Gaussian.
    float blur_sigma = mat->sigma;
    for (int i = 0; i < 4; ++i) {
        const auto& corner = mat->corner_texture(i);
        if (corner.texture) {
            blur_sigma = corner.sigma;
            break;
        }
    }
    memcpy(buf->data() + 68, &blur_sigma, 4);
    const float rect_w = mat->rect_size.x();
    const float rect_h = mat->rect_size.y();
    memcpy(buf->data() + 72, &rect_w, 4);
    memcpy(buf->data() + 76, &rect_h, 4);
    const int style_i = static_cast<int>(mat->style);
    memcpy(buf->data() + 80, &style_i, 4);
    const float r = mat->effective_radius();
    memcpy(buf->data() + 84, &r, 4);

    const float r_tl = mat->radius.x();
    const float r_tr = mat->radius.y();
    const float r_bl = mat->radius.z();
    const float r_br = mat->radius.w();
    memcpy(buf->data() + 88, &r_tl, 4);
    memcpy(buf->data() + 92, &r_tr, 4);
    memcpy(buf->data() + 96, &r_bl, 4);
    memcpy(buf->data() + 100, &r_br, 4);
    for (int i = 0; i < 4; ++i) {
        const auto& corner = mat->corner_texture(i);
        const float mapping[] { corner.margin, corner.texture ? corner.extent : 0.0f, 0, 0 };
        memcpy(buf->data() + 112 + i * 16, mapping, sizeof(mapping));
    }
    return true;
}

void BlurMaskShader::updateSampledImage(RenderState& state, int binding, QSGTexture** texture,
                                        QSGMaterial* newMaterial, QSGMaterial*) {
    auto        mat = static_cast<BlurMaskMaterial*>(newMaterial);
    QSGTexture* t   = nullptr;
    if (binding == 1) {
        t = mat->profile_texture();
    } else if (binding >= 2 && binding <= 5) {
        t = mat->corner_texture(binding - 2).texture;
        if (! t) t = mat->profile_texture(); // dummy fallback to keep the pipeline happy
    }
    if (t) {
        *texture = t;
        t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    }
}

} // namespace qml_material::sg
