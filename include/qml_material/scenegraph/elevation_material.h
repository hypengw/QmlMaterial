#pragma once

#include <QSGMaterial>
#include <array>
#include <QRectF>
#include <QVector4D>

namespace qml_material::sg
{

struct ShadowOpData {
    bool      enabled { false };
    QRectF    outerRect;   // quad bounds (including blur)
    QRectF    innerRect;   // occluder bounds (no blur)
    QVector4D radius;      // tl,tr,bl,br for innerRect
    float     blurOutset { 0.0f }; // penumbra width in pixels
    QRgb      color { 0 };
};

class ElevationMaterial : public QSGMaterial {
public:
    ElevationMaterial();
    ~ElevationMaterial();

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    QSGMaterialType*   type() const override;
    int                compare(const QSGMaterial* other) const override;

    void init_fadeoff_texture(QQuickWindow* win);
    auto strength_texture() -> QSGTexture*;

    void setShadowOps(const std::array<ShadowOpData, 2>& ops) { m_ops = ops; }
    auto shadowOps() const noexcept -> const std::array<ShadowOpData, 2>& { return m_ops; }

private:
    QSGTexture* m_fadeoff_texture;
    std::array<ShadowOpData, 2> m_ops;
};

class ElevationShader : public QSGMaterialShader {
public:
    ElevationShader();

    bool updateUniformData(QSGMaterialShader::RenderState& state, QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;

    void updateSampledImage(RenderState&, int binding, QSGTexture** texture,
                            QSGMaterial* newMaterial, QSGMaterial*) override;
};

} // namespace qml_material::sg
