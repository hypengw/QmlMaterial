#pragma once

#include <QtCore/QObject>
#include <QtGui/QColor>

#include "qml_material/core/enum.hpp"

#include "cpp/palettes/tones.h"

namespace qml_material
{
#define QM_SCHEME_ROLES(X)        \
    X(primary)                    \
    X(on_primary)                 \
    X(primary_container)          \
    X(on_primary_container)       \
    X(primary_fixed)              \
    X(primary_fixed_dim)          \
    X(on_primary_fixed)           \
    X(on_primary_fixed_variant)   \
    X(secondary)                  \
    X(on_secondary)               \
    X(secondary_container)        \
    X(on_secondary_container)     \
    X(secondary_fixed)            \
    X(secondary_fixed_dim)        \
    X(on_secondary_fixed)         \
    X(on_secondary_fixed_variant) \
    X(tertiary)                   \
    X(on_tertiary)                \
    X(tertiary_container)         \
    X(on_tertiary_container)      \
    X(tertiary_fixed)             \
    X(tertiary_fixed_dim)         \
    X(on_tertiary_fixed)          \
    X(on_tertiary_fixed_variant)  \
    X(error)                      \
    X(on_error)                   \
    X(error_container)            \
    X(on_error_container)         \
    X(background)                 \
    X(on_background)              \
    X(surface)                    \
    X(on_surface)                 \
    X(surface_variant)            \
    X(on_surface_variant)         \
    X(outline)                    \
    X(outline_variant)            \
    X(shadow)                     \
    X(scrim)                      \
    X(inverse_surface)            \
    X(inverse_on_surface)         \
    X(inverse_primary)            \
    X(surface_1)                  \
    X(surface_2)                  \
    X(surface_3)                  \
    X(surface_4)                  \
    X(surface_5)                  \
    X(surface_dim)                \
    X(surface_bright)             \
    X(surface_container)          \
    X(surface_container_low)      \
    X(surface_container_lowest)   \
    X(surface_container_high)     \
    X(surface_container_highest)

struct MdScheme {
#define X(_n_) QRgb _n_;
    QM_SCHEME_ROLES(X)
#undef X

    material_color_utilities::TonalPalette neutral_palette { 0 };
};

auto material_light_color_scheme(QRgb, Enum::PaletteType) -> MdScheme;
auto material_dark_color_scheme(QRgb, Enum::PaletteType) -> MdScheme;

auto material_blend_hcthue(const QRgb design_color, const QRgb key_color, const double mount)
    -> QRgb;
auto color_from_image(const QImage&) -> QRgb;
} // namespace qml_material
