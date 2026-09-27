#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/button.hpp")
namespace qml_material
{
class Button;
class QML_MATERIAL_API RailItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateRailItem)
    Q_PROPERTY(qml_material::Button* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(QColor collapsedLabelColor READ collapsedLabelColor NOTIFY collapsedLabelColorChanged
                   BINDABLE bindableCollapsedLabelColor FINAL)
    Q_PROPERTY(QColor expandedLabelColor READ expandedLabelColor NOTIFY expandedLabelColorChanged
                   BINDABLE bindableExpandedLabelColor FINAL)
public:
    explicit RailItemState(QObject* parent = nullptr);
    Button*           item() const;
    void              setItem(Button*);
    QColor            collapsedLabelColor() const;
    QColor            expandedLabelColor() const;
    QBindable<QColor> bindableCollapsedLabelColor() const;
    QBindable<QColor> bindableExpandedLabelColor() const;
    Q_SIGNAL void     collapsedLabelColorChanged();
    Q_SIGNAL void     expandedLabelColorChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(RailItemState, QColor, m_collapsedLabelColor,
                               &RailItemState::collapsedLabelColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(RailItemState, QColor, m_expandedLabelColor,
                               &RailItemState::expandedLabelColorChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
