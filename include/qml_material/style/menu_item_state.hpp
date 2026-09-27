#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API MenuItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateMenuItem)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged BINDABLE
                   bindableSelected FINAL)
    Q_PROPERTY(QColor leadingIconColor READ leadingIconColor WRITE setLeadingIconColor RESET
                   resetLeadingIconColor NOTIFY leadingIconColorChanged BINDABLE
                       bindableLeadingIconColor FINAL)
    Q_PROPERTY(QColor trailingIconColor READ trailingIconColor WRITE setTrailingIconColor RESET
                   resetTrailingIconColor NOTIFY trailingIconColorChanged BINDABLE
                       bindableTrailingIconColor FINAL)
public:
    explicit MenuItemState(QObject* parent = nullptr);
    Button*           item() const;
    void              setItem(Button*);
    bool              selected() const;
    void              setSelected(bool);
    QBindable<bool>   bindableSelected();
    QColor            leadingIconColor() const;
    void              setLeadingIconColor(const QColor&);
    Q_INVOKABLE void  resetLeadingIconColor();
    QBindable<QColor> bindableLeadingIconColor();
    QColor            trailingIconColor() const;
    void              setTrailingIconColor(const QColor&);
    Q_INVOKABLE void  resetTrailingIconColor();
    QBindable<QColor> bindableTrailingIconColor();
    Q_SIGNAL void     selectedChanged();
    Q_SIGNAL void     leadingIconColorChanged();
    Q_SIGNAL void     trailingIconColorChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MenuItemState, bool, m_selected, false,
                                         &MenuItemState::selectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(MenuItemState, QColor, m_leadingIconColor,
                               &MenuItemState::leadingIconColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(MenuItemState, QColor, m_trailingIconColor,
                               &MenuItemState::trailingIconColorChanged)
    PropertyKey<QColor> m_leadingKey, m_trailingKey;
    BindingLifetime     m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
