#include "qml_material/token/color.hpp"

#include <QtGui/QGuiApplication>
#include <QStyleHints>

#include "qml_material/core/helper.hpp"
#include "qml_material/util/qml_util.hpp"

using namespace qcm;
using namespace qml_material;

namespace
{
constexpr QRgb BASE_COLOR { qRgb(190, 231, 253) };
} // namespace

MdColorMgr::MdColorMgr(QObject* parent)
    : QObject(parent),
      m_accent_color(BASE_COLOR),
      m_mode(sysColorScheme()),
      m_last_mode(m_mode),
      m_scheme_type(Enum::PaletteType::PaletteNeutral),
      m_use_sys_color_scheme(true),
      m_use_sys_accent_color(false) {
    genScheme();
    connect(this, &Self::modeChanged, this, &Self::genScheme);
    connect(this, &Self::accentColorChanged, this, &Self::genScheme);
    connect(this, &Self::paletteTypeChanged, this, &Self::genScheme);

    sysNotifyInit(*this);

    connect(this, &Self::useSysColorSMChanged, this, &Self::refrehFromSystem);
    connect(this, &Self::useSysAccentColorChanged, this, &Self::refrehFromSystem);
}

Enum::ThemeMode MdColorMgr::sysColorScheme() const { return ::sysColorScheme(); }
QColor          MdColorMgr::sysAccentColor() const { return ::sysAccentColor(); }

Enum::ThemeMode MdColorMgr::mode() const { return m_mode; }
void            MdColorMgr::setMode(Enum::ThemeMode v) {
    if (std::exchange(m_mode, v) != v) {
        Q_EMIT modeChanged(m_mode);
    }
}

QColor MdColorMgr::accentColor() const { return m_accent_color; }

bool MdColorMgr::useSysColorSM() const { return m_use_sys_color_scheme; };
bool MdColorMgr::useSysAccentColor() const { return m_use_sys_accent_color; };
auto MdColorMgr::paletteType() const -> Enum::PaletteType { return m_scheme_type; }
void MdColorMgr::setPaletteType(Enum::PaletteType t) {
    if (std::exchange(m_scheme_type, t) != t) {
        paletteTypeChanged();
    }
}

void MdColorMgr::setAccentColor(QColor v) {
    if (std::exchange(m_accent_color, v) != v) {
        Q_EMIT accentColorChanged(m_accent_color);
    }
}

void MdColorMgr::setUseSysColorSM(bool v) {
    if (std::exchange(m_use_sys_color_scheme, v) != v) {
        Q_EMIT useSysColorSMChanged();
    }
}

void MdColorMgr::setUseSysAccentColor(bool v) {
    if (std::exchange(m_use_sys_accent_color, v) != v) {
        Q_EMIT useSysAccentColorChanged();
    }
}

QColor MdColorMgr::getOn(QColor in) const {
    const auto& s = m_scheme;
    const QRgb  v = in.rgb();

    if (v == s.primary) return s.on_primary;
    if (v == s.primary_container) return s.on_primary_container;
    if (v == s.primary_fixed || v == s.primary_fixed_dim) return s.on_primary_fixed;
    if (v == s.secondary) return s.on_secondary;
    if (v == s.secondary_container) return s.on_secondary_container;
    if (v == s.secondary_fixed || v == s.secondary_fixed_dim) return s.on_secondary_fixed;
    if (v == s.tertiary) return s.on_tertiary;
    if (v == s.tertiary_container) return s.on_tertiary_container;
    if (v == s.tertiary_fixed || v == s.tertiary_fixed_dim) return s.on_tertiary_fixed;
    if (v == s.error) return s.on_error;
    if (v == s.error_container) return s.on_error_container;
    if (v == s.inverse_surface || v == s.inverse_primary) return s.inverse_on_surface;
    if (v == s.surface_variant) return s.on_surface_variant;
    if (v == s.background) return s.on_background;

    // surface / surface_container_* / surface_1..5 all share on_surface
    return s.on_surface;
}

void MdColorMgr::genSchemeImpl(Enum::ThemeMode mode) {
    if (mode == Enum::ThemeMode::Light)
        m_scheme = material_light_color_scheme(m_accent_color.rgb(), paletteType());
    else
        m_scheme = material_dark_color_scheme(m_accent_color.rgb(), paletteType());

    m_last_mode = mode;

    Q_EMIT schemeChanged();
}

void MdColorMgr::genScheme() {
    auto m = useSysColorSM() ? sysColorScheme() : mode();
    genSchemeImpl(m);
}

void MdColorMgr::refrehFromSystem() {
    if (useSysColorSM()) {
        genSchemeImpl(sysColorScheme());
    } else {
        if (m_last_mode != mode()) {
            genScheme();
        }
    }

    if (useSysAccentColor()) {
        setAccentColor(sysAccentColor());
    }
}

void MdColorMgr::selectFromImage(const QImage& image) {
    auto color = color_from_image(image);
    setUseSysAccentColor(false);
    setAccentColor(color);
}

#include <qml_material/token/moc_color.cpp>