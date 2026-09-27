#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API StandardIconButtonState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateStandardIconButton)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)

public:
    explicit StandardIconButtonState(QObject* parent = nullptr);
    Button* item() const;
    void    setItem(Button*);

private:
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
