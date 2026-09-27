#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/radio_button.hpp")
namespace qml_material
{
class RadioButton;
class QML_MATERIAL_API RadioButtonState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateRadioButton)
    Q_PROPERTY(qml_material::RadioButton* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor RESET resetIconColor NOTIFY
                   iconColorChanged BINDABLE bindableIconColor FINAL)

public:
    explicit RadioButtonState(QObject* parent = nullptr);
    RadioButton*      item() const;
    void              setItem(RadioButton*);
    QColor            iconColor() const;
    void              setIconColor(const QColor&);
    QBindable<QColor> bindableIconColor();
    Q_INVOKABLE void  resetIconColor();
    Q_SIGNAL void     iconColorChanged();

private:
    PropertyKey<QColor> m_iconKey;
    Q_OBJECT_BINDABLE_PROPERTY(RadioButtonState, QColor, m_iconColor,
                               &RadioButtonState::iconColorChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
