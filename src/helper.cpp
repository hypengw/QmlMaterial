#include "qml_material/helper.h"

#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/blend/blend.h"

#include "qml_material/core.h"

#include "cpp/palettes/core.h"

using MdScheme = qcm::MdScheme;
namespace md   = material_color_utilities;

namespace
{

QRgb blend(QRgb a, QRgb b, double t) {
    double dt = 1.0 - t;
    return qRgba(qRed(a) * dt + qRed(b) * t,
                 qGreen(a) * dt + qGreen(b) * t,
                 qBlue(a) * dt + qBlue(b) * t,
                 qAlpha(a));
}
} // namespace

static void convert_from(MdScheme& out, const md::DynamicScheme& in) {
    out.primary                = in.GetPrimary();
    out.on_primary             = in.GetOnPrimary();
    out.primary_container      = in.GetPrimaryContainer();
    out.on_primary_container   = in.GetOnPrimaryContainer();
    out.secondary              = in.GetSecondary();
    out.on_secondary           = in.GetOnSecondary();
    out.secondary_container    = in.GetSecondaryContainer();
    out.on_secondary_container = in.GetOnSecondaryContainer();
    out.tertiary               = in.GetTertiary();
    out.on_tertiary            = in.GetOnTertiary();
    out.tertiary_container     = in.GetTertiaryContainer();
    out.on_tertiary_container  = in.GetOnTertiaryContainer();
    out.error                  = in.GetError();
    out.on_error               = in.GetOnError();
    out.error_container        = in.GetErrorContainer();
    out.on_error_container     = in.GetOnErrorContainer();
    out.background             = in.GetBackground();
    out.on_background          = in.GetOnBackground();
    out.surface                = in.GetSurface();
    out.on_surface             = in.GetOnSurface();
    out.surface_variant        = in.GetSurfaceVariant();
    out.on_surface_variant     = in.GetOnSurfaceVariant();
    out.outline                = in.GetOutline();
    out.outline_variant        = in.GetOutlineVariant();
    out.shadow                 = in.GetShadow();
    out.scrim                  = in.GetScrim();
    out.inverse_surface        = in.GetInverseSurface();
    out.inverse_on_surface     = in.GetInverseOnSurface();
    out.inverse_primary        = in.GetInversePrimary();

    out.surface_dim               = in.GetSurfaceDim();
    out.surface_bright            = in.GetSurfaceBright();
    out.surface_container         = in.GetSurfaceContainer();
    out.surface_container_low     = in.GetSurfaceContainerLow();
    out.surface_container_lowest  = in.GetSurfaceContainerLowest();
    out.surface_container_high    = in.GetSurfaceContainerHigh();
    out.surface_container_highest = in.GetSurfaceContainerHighest();

    out.surface_1 = blend(out.surface, out.primary, 0.05);
    out.surface_2 = blend(out.surface, out.primary, 0.08);
    out.surface_3 = blend(out.surface, out.primary, 0.11);
    out.surface_4 = blend(out.surface, out.primary, 0.12);
    out.surface_5 = blend(out.surface, out.primary, 0.14);
}

MdScheme qcm::MaterialLightColorScheme(QRgb rgb) {
    md::Hct  hct(rgb);
    auto     scheme_content = md::SchemeTonalSpot(hct, false, 0.0);
    MdScheme scheme;
    convert_from(scheme, scheme_content);
    return scheme;
}

MdScheme qcm::MaterialDarkColorScheme(QRgb rgb) {
    md::Hct  hct(rgb);
    auto     scheme_content = md::SchemeTonalSpot(hct, true);
    MdScheme scheme;
    convert_from(scheme, scheme_content);
    return scheme;
}

QRgb qcm::MaterialBlendHctHue(const QRgb design_color, const QRgb key_color, const double mount) {
    return md::BlendHctHue(design_color, key_color, mount);
}
