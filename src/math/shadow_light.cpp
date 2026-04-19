#include "qml_material/math/shadow_light.hpp"

namespace qml_material::math
{

void spot_params(scalar occluder_z, scalar light_x, scalar light_y, scalar light_z,
                 scalar light_radius, scalar* blur_radius, scalar* scale,
                 QVector2D* translate) {
    const scalar z_ratio = divide_and_pin(occluder_z, light_z - occluder_z, 0.0f, 0.95f);
    *blur_radius = light_radius * z_ratio;
    *scale       = divide_and_pin(light_z, light_z - occluder_z, 1.0f, 1.95f);
    *translate   = QVector2D(-z_ratio * light_x, -z_ratio * light_y);
}

void directional_params(scalar occluder_z, scalar light_x, scalar light_y, scalar light_z,
                        scalar light_radius, scalar* blur_radius, scalar* scale,
                        QVector2D* translate) {
    *blur_radius = light_radius * occluder_z;
    *scale       = 1;
    constexpr scalar k_max_z_ratio = 64 / k_nearly_zero;
    const scalar z_ratio = divide_and_pin(occluder_z, light_z, 0.0f, k_max_z_ratio);
    *translate = QVector2D(-z_ratio * light_x, -z_ratio * light_y);
}

} // namespace qml_material::math
