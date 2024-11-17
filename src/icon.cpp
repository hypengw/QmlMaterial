#include "qml_material/icon.h"

namespace qml_material
{
auto token::create_icon_token(QObject* parent) -> IconToken* { return new IconToken(parent); }
} // namespace qml_material