#pragma once

#include <QSGMaterial>

namespace qml_material::sg
{

class ShadowMaterial : public QSGMaterial {
public:
    ShadowMaterial();
    ~ShadowMaterial();

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    QSGMaterialType*   type() const override;
    int                compare(const QSGMaterial* other) const override;

    void init_fadeoff_texture(QQuickWindow* win);
    auto strength_texture() -> QSGTexture*;

private:
    QSGTexture* m_fadeoff_texture;
};

class ShadowShader : public QSGMaterialShader {
public:
    ShadowShader();

    bool updateUniformData(QSGMaterialShader::RenderState& state, QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;

    void updateSampledImage(RenderState&, int binding, QSGTexture** texture,
                            QSGMaterial* newMaterial, QSGMaterial*) override;
};

} // namespace qml_material::sg
