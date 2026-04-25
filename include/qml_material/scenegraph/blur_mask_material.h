#pragma once

#include <QSGMaterial>
#include <QVector2D>
#include <QVector4D>

namespace qml_material::sg
{

enum class BlurStyle : int {
    Normal = 0,
    Solid  = 1,
    Outer  = 2,
    Inner  = 3,
};

/**
 * QSGMaterial that renders a gaussian-blurred rect / rrect mask analytically
 * using a 1D cumulative-gaussian profile texture.
 *
 * The profile is unit-sigma and sigma-independent — one texture serves all σ.
 * Samples are taken in the shader as `Φ(t/σ) ≈ profile((t/σ)/6 + 0.5)`.
 */
class BlurMaskMaterial : public QSGMaterial {
public:
    BlurMaskMaterial();
    ~BlurMaskMaterial() = default;

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    QSGMaterialType*   type() const override;
    int                compare(const QSGMaterial* other) const override;

    /// Bind the per-window shared 1D unit-CDF profile texture. Cheap.
    void init_profile_texture(QQuickWindow* win);
    auto profile_texture() -> QSGTexture*;

    /// Resolve a (sigma, radius)-keyed corner blur texture from the per-window
    /// shared cache. No-op below the analytic-path threshold.
    void init_corner_texture(QQuickWindow* win, float sigma, float radius);
    auto corner_texture() -> QSGTexture*;

    // Uniform state (populated by the owning node each frame).
    float     sigma { 0.0f };
    QVector2D rect_size { 0.0f, 0.0f };
    QVector4D radius { 0.0f, 0.0f, 0.0f, 0.0f }; // TL, TR, BL, BR
    BlurStyle style { BlurStyle::Normal };

    /// Effective uniform radius used by the shader (= max of the 4 radii).
    [[nodiscard]] float effective_radius() const;

private:
    // Not owned — both pointers come from the per-window texture cache.
    QSGTexture* m_profile_texture { nullptr };
    QSGTexture* m_corner_texture { nullptr };
};

class BlurMaskShader : public QSGMaterialShader {
public:
    BlurMaskShader();

    bool updateUniformData(QSGMaterialShader::RenderState& state, QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;

    void updateSampledImage(RenderState&, int binding, QSGTexture** texture,
                            QSGMaterial* newMaterial, QSGMaterial*) override;
};

} // namespace qml_material::sg
