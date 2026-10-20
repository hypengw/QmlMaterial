#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/selection_button.hpp")
namespace qml_material
{
class SelectionButton;
class QML_MATERIAL_API BarItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateBarItem)
    Q_PROPERTY(qml_material::SelectionButton* item READ item WRITE setItem NOTIFY itemChanged FINAL)

public:
    explicit BarItemState(QObject* parent = nullptr);
    SelectionButton* item() const;
    void             setItem(SelectionButton*);

private:
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
