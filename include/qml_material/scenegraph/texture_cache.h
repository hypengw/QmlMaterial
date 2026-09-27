#pragma once

#include <QSGTexture>
#include <QQuickWindow>

namespace qml_material::sg
{

/// Returns a per-window shared 128x1 grayscale unit-CDF profile texture used
/// by BlurMaskMaterial. Identical for every BlurMask instance.
/// Ownership stays with the cache; callers must NOT delete the returned pointer.
QSGTexture* shared_blur_profile_texture(QQuickWindow* win);

/// Returns a per-window shared 128x1 grayscale shadow fadeoff texture used by
/// ShadowMaterial. Identical for every RRectShadow instance.
/// Ownership stays with the cache; callers must NOT delete the returned pointer.
QSGTexture* shared_shadow_fadeoff_texture(QQuickWindow* win);

struct CornerCutoutTexture {
    QSGTexture* texture = nullptr;
    float       margin  = 0;
    float       extent  = 0;
    float       sigma   = 0;
};

/// Shared blurred corner cutout and its local-coordinate mapping, keyed by
/// quantised (sigma, radius). Ownership stays with the per-window cache.
CornerCutoutTexture shared_rrect_corner_cutout_texture(QQuickWindow* win, float sigma,
                                                       float radius);

} // namespace qml_material::sg
