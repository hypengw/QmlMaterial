#include "qml_material/scenegraph/shadow_material.h"

#include <cmath>

#include <QImage>
#include <QQuickWindow>
#include <QSGTexture>

#include "qml_material/math/scalar.hpp"

namespace qml_material::sg
{

ShadowMaterial::ShadowMaterial(): m_fadeoff_texture(nullptr) {
    setFlag(QSGMaterial::Blending, true);
}
ShadowMaterial::~ShadowMaterial() { delete m_fadeoff_texture; }

QSGMaterialShader* ShadowMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new ShadowShader {};
}

QSGMaterialType* ShadowMaterial::type() const {
    static QSGMaterialType staticType;
    return &staticType;
}

int ShadowMaterial::compare(const QSGMaterial* other) const {
    return QSGMaterial::compare(other);
}

auto ShadowMaterial::strength_texture() -> QSGTexture* { return m_fadeoff_texture; }
void ShadowMaterial::init_fadeoff_texture(QQuickWindow* win) {
    const int width  = 128;
    const int height = 1;
    Q_UNUSED(height);
    QImage image(width, 1, QImage::Format_Grayscale8);

    uchar* row = image.scanLine(0);
    for (int i = 0; i < 128; i++) {
        float d = math::k_scalar_one - i / 127.0f;
        row[i]  = math::round_to_int(((float)std::exp(-4 * d * d) - 0.018f) * 255);
    }

    QSGTexture* texture = win->createTextureFromImage(image);
    m_fadeoff_texture   = texture;
}

ShadowShader::ShadowShader() {
    setShaderFileName(QSGMaterialShader::VertexStage,
                      QLatin1String(":/Qcm/Material/assets/shader/shadow.vert.qsb"));
    setShaderFileName(QSGMaterialShader::FragmentStage,
                      QLatin1String(":/Qcm/Material/assets/shader/shadow.frag.qsb"));
}

bool ShadowShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial,
                                     QSGMaterial* oldMaterial) {
    Q_UNUSED(newMaterial);
    Q_UNUSED(oldMaterial);
    bool        changed = false;
    QByteArray* buf     = state.uniformData();
    Q_ASSERT(buf->size() >= 68);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = std::pow(state.opacity(), 3);
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }
    return changed;
}

void ShadowShader::updateSampledImage(RenderState& state, int binding, QSGTexture** texture,
                                      QSGMaterial* newMaterial, QSGMaterial*) {
    auto mat = static_cast<ShadowMaterial*>(newMaterial);
    if (binding == 1) {
        *texture = mat->strength_texture();
        texture[0]->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    }
}

} // namespace qml_material::sg
