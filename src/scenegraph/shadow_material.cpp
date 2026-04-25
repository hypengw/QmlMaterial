#include "qml_material/scenegraph/shadow_material.h"

#include <cmath>

#include <QQuickWindow>
#include <QSGTexture>

#include "qml_material/scenegraph/texture_cache.h"

namespace qml_material::sg
{

ShadowMaterial::ShadowMaterial() {
    setFlag(QSGMaterial::Blending, true);
}

QSGMaterialShader* ShadowMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new ShadowShader {};
}

QSGMaterialType* ShadowMaterial::type() const {
    static QSGMaterialType staticType;
    return &staticType;
}

int ShadowMaterial::compare(const QSGMaterial* other) const {
    // type() is checked by Qt before compare() runs, so `other` is a
    // ShadowMaterial. The only per-instance state visible to the GPU is the
    // fadeoff sampler, which is shared via the per-window cache — every
    // instance bound to the same window collapses to the same pointer and
    // batches together.
    auto* o = static_cast<const ShadowMaterial*>(other);
    if (m_fadeoff_texture == o->m_fadeoff_texture) return 0;
    return m_fadeoff_texture < o->m_fadeoff_texture ? -1 : 1;
}

auto ShadowMaterial::strength_texture() -> QSGTexture* { return m_fadeoff_texture; }

void ShadowMaterial::init_fadeoff_texture(QQuickWindow* win) {
    m_fadeoff_texture = shared_shadow_fadeoff_texture(win);
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
