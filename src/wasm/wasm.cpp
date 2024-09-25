#include "qml_material/util.h"
#include "qml_material/color.h"

namespace qml_material
{
void sys_notify(MdColorMgr&) {}
auto sysColorScheme() -> MdColorMgr::ColorSchemeEnum { return {}; }
auto sysAccentColor() -> QColor { return {}; }

} // namespace qml_material