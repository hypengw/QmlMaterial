#include "qml_material/style/material_state.hpp"

namespace qml_material
{
MaterialState::MaterialState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
}
} // namespace qml_material
