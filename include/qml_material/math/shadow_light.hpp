#pragma once

/*
 * Shadow-light parameter math.
 *
 * Derived from Skia's analytic shadow model (SkShadowUtils, SkGpuBlurUtils).
 * Skia is BSD-3 licensed; see third_party/skia/LICENSE. This is an independent
 * re-expression, not a literal copy.
 */

#include <QVector2D>

#include "qml_material/math/scalar.hpp"

namespace qml_material::math
{

inline constexpr scalar k_ambient_height_factor = 1.0f / 128.0f;
inline constexpr scalar k_ambient_geom_factor   = 64.0f;
inline constexpr scalar k_max_ambient_radius    = 300 * k_ambient_height_factor * k_ambient_geom_factor;

[[nodiscard]] inline scalar ambient_blur_radius(scalar height) {
    return std::min(height * k_ambient_height_factor * k_ambient_geom_factor, k_max_ambient_radius);
}

[[nodiscard]] inline scalar ambient_recip_alpha(scalar height) {
    return 1.0f + std::max(height * k_ambient_height_factor, 0.0f);
}

void spot_params(scalar occluder_z, scalar light_x, scalar light_y, scalar light_z,
                 scalar light_radius, scalar* blur_radius, scalar* scale,
                 QVector2D* translate);

void directional_params(scalar occluder_z, scalar light_x, scalar light_y, scalar light_z,
                        scalar light_radius, scalar* blur_radius, scalar* scale,
                        QVector2D* translate);

} // namespace qml_material::math
