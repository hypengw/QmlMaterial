#pragma once
#include <QLocale>
#include "qml_material/export.hpp"

namespace qml_material::time_format
{
QML_MATERIAL_API bool    uses24Hour(const QLocale&);
QML_MATERIAL_API QString number(const QLocale&, int value, bool padded);
} // namespace qml_material::time_format
