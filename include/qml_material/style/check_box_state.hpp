#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/check_box.hpp")
namespace qml_material
{
class CheckBox;
class QML_MATERIAL_API CheckBoxState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateCheckBox)
    Q_PROPERTY(qml_material::CheckBox* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(
        bool error READ error WRITE setError NOTIFY errorChanged BINDABLE bindableError FINAL)
    Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor RESET resetIconColor NOTIFY
                   iconColorChanged BINDABLE bindableIconColor FINAL)
    Q_PROPERTY(QColor iconBackgroundColor READ iconBackgroundColor WRITE setIconBackgroundColor
                   RESET resetIconBackgroundColor NOTIFY iconBackgroundColorChanged BINDABLE
                       bindableIconBackgroundColor FINAL)

public:
    explicit CheckBoxState(QObject* parent = nullptr);
    CheckBox*         item() const;
    void              setItem(CheckBox*);
    bool              error() const;
    void              setError(bool);
    QBindable<bool>   bindableError();
    Q_SIGNAL void     errorChanged();
    QColor            iconColor() const;
    void              setIconColor(const QColor&);
    QBindable<QColor> bindableIconColor();
    Q_INVOKABLE void  resetIconColor();
    Q_SIGNAL void     iconColorChanged();
    QColor            iconBackgroundColor() const;
    void              setIconBackgroundColor(const QColor&);
    QBindable<QColor> bindableIconBackgroundColor();
    Q_INVOKABLE void  resetIconBackgroundColor();
    Q_SIGNAL void     iconBackgroundColorChanged();

private:
    PropertyKey<QColor> m_iconKey, m_iconBackgroundKey;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CheckBoxState, bool, m_error, false,
                                         &CheckBoxState::errorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CheckBoxState, QColor, m_iconColor, &CheckBoxState::iconColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CheckBoxState, QColor, m_iconBackgroundColor,
                               &CheckBoxState::iconBackgroundColorChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
