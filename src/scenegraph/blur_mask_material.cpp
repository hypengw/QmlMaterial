#include "qml_material/scenegraph/blur_mask_material.h"

#include <cstring>
#include <vector>

#include <QImage>
#include <QQuickWindow>
#include <QSGTexture>

#include "qml_material/math/gaussian.hpp"

namespace qml_material::sg
{

namespace
{
constexpr int  kProfileSize  = 128;
constexpr auto kVertPath     = ":/Qcm/Material/assets/shader/blur_mask.vert.qsb";
constexpr auto kFragPath     = ":/Qcm/Material/assets/shader/blur_mask.frag.qsb";

constexpr float kMinSigma  = 0.5f;
constexpr float kMinRadius = 0.5f;
} // namespace

BlurMaskMaterial::BlurMaskMaterial() { setFlag(QSGMaterial::Blending, true); }
BlurMaskMaterial::~BlurMaskMaterial() {
    delete m_profile_texture;
    delete m_corner_texture;
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
    return QSGMaterial::compare(other);
}

auto BlurMaskMaterial::profile_texture() -> QSGTexture* { return m_profile_texture; }
auto BlurMaskMaterial::corner_texture() -> QSGTexture* { return m_corner_texture; }

void BlurMaskMaterial::init_profile_texture(QQuickWindow* win) {
    if (m_profile_texture) return;
    QImage image(kProfileSize, 1, QImage::Format_Grayscale8);
    math::fill_unit_cdf_profile(
        std::span<std::uint8_t> { image.scanLine(0), static_cast<std::size_t>(kProfileSize) });
    m_profile_texture = win->createTextureFromImage(image);
}

void BlurMaskMaterial::init_corner_texture(QQuickWindow* win, float sigma_, float radius_) {
    const corner_key key {
        .sigma_q  = static_cast<int>(std::round(sigma_ * 2.0f)),
        .radius_q = static_cast<int>(std::round(radius_ * 2.0f)),
    };
    if (m_corner_key && *m_corner_key == key) return;

    delete m_corner_texture;
    m_corner_texture = nullptr;
    m_corner_key     = key;

    if (sigma_ < kMinSigma || radius_ < kMinRadius) return;

    const int n = math::rrect_corner_blur_size(sigma_, radius_);
    if (n <= 0) return;

    QImage image(n, n, QImage::Format_Grayscale8);
    // Grayscale8 guarantees 1 byte per pixel; QImage may pad rows, so copy per-row.
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(n) * n);
    math::fill_rrect_corner_blur(buf, sigma_, radius_);
    for (int j = 0; j < n; ++j) {
        std::memcpy(image.scanLine(j), buf.data() + static_cast<std::size_t>(j) * n,
                    static_cast<std::size_t>(n));
    }
    m_corner_texture = win->createTextureFromImage(image);
}

BlurMaskShader::BlurMaskShader() {
    setShaderFileName(QSGMaterialShader::VertexStage, QLatin1String(kVertPath));
    setShaderFileName(QSGMaterialShader::FragmentStage, QLatin1String(kFragPath));
}

bool BlurMaskShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial,
                                       QSGMaterial* /*oldMaterial*/) {
    auto*       mat     = static_cast<BlurMaskMaterial*>(newMaterial);
    bool        changed = false;
    QByteArray* buf     = state.uniformData();
    // std140 block: mat4(64) + float(4) + float(4) + vec2(8) + int(4) + float(4) = 88 bytes
    Q_ASSERT(buf->size() >= 88);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }
    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }

    // Always write the non-engine uniforms; cost is negligible.
    memcpy(buf->data() + 68, &mat->sigma, 4);
    const float rect_w = mat->rect_size.x();
    const float rect_h = mat->rect_size.y();
    memcpy(buf->data() + 72, &rect_w, 4);
    memcpy(buf->data() + 76, &rect_h, 4);
    const int style_i = static_cast<int>(mat->style);
    memcpy(buf->data() + 80, &style_i, 4);
    const float r = mat->effective_radius();
    memcpy(buf->data() + 84, &r, 4);
    changed = true;

    return changed;
}

void BlurMaskShader::updateSampledImage(RenderState& state, int binding, QSGTexture** texture,
                                        QSGMaterial* newMaterial, QSGMaterial*) {
    auto mat = static_cast<BlurMaskMaterial*>(newMaterial);
    QSGTexture* t = nullptr;
    if (binding == 1) {
        t = mat->profile_texture();
    } else if (binding == 2) {
        t = mat->corner_texture();
        if (!t) t = mat->profile_texture(); // dummy fallback to keep the pipeline happy
    }
    if (t) {
        *texture = t;
        t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    }
}

} // namespace qml_material::sg
