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

/// Returns a per-window shared rrect-corner blur texture, keyed by quantised
/// (sigma, radius). Returns nullptr below the analytic threshold.
/// Ownership stays with the cache; callers must NOT delete the returned pointer.
QSGTexture* shared_rrect_corner_blur_texture(QQuickWindow* win, float sigma, float radius);

} // namespace qml_material::sg
