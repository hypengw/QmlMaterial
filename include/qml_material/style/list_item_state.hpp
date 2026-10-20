#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button_control.hpp")
namespace qml_material
{
class ButtonControl;
class QML_MATERIAL_API ListItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateListItem)
    Q_PROPERTY(qml_material::ButtonControl* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged BINDABLE
                   bindableSelected FINAL)
public:
    explicit ListItemState(QObject* parent = nullptr);
    ButtonControl*  item() const;
    void            setItem(ButtonControl*);
    bool            selected() const { return m_selected; }
    void            setSelected(bool value) { m_selected = value; }
    QBindable<bool> bindableSelected() { return &m_selected; }
    Q_SIGNAL void   selectedChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(ListItemState, bool, m_selected, &ListItemState::selectedChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
