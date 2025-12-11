#include "qml_material/util/qml_util.hpp"
#include "qml_material/token/color.hpp"

#include <QGuiApplication>
#include <QStyleHints>
#include <QObject>

namespace qml_material
{
void sysNotifyInit(MdColorMgr& mgr) {
    if (auto* hints = QGuiApplication::styleHints()) {
        QObject::connect(
            hints,
            &QStyleHints::colorSchemeChanged,
            &mgr,
            &MdColorMgr::refrehFromSystem);
    }
}

auto sysColorScheme() -> Enum::ThemeMode {
    if (auto* hints = QGuiApplication::styleHints()) {
        const auto scheme = hints->colorScheme();
        return scheme == Qt::ColorScheme::Dark ? Enum::ThemeMode::Dark : Enum::ThemeMode::Light;
    }
    return Enum::ThemeMode::Light;
}

auto sysAccentColor() -> QColor { return {}; }

} // namespace qml_material
