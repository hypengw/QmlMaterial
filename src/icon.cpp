#include "qml_material/icon.h"

namespace qml_material
{
auto token::create_icon_token() -> IconToken* { return new IconToken(); }
} // namespace qml_material