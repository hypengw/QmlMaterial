#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/selection_button.hpp")
namespace qml_material
{
class SelectionButton;
class QML_MATERIAL_API RailItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateRailItem)
    Q_PROPERTY(qml_material::SelectionButton* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(QColor collapsedLabelColor READ collapsedLabelColor NOTIFY collapsedLabelColorChanged
                   BINDABLE bindableCollapsedLabelColor FINAL)
    Q_PROPERTY(QColor expandedLabelColor READ expandedLabelColor NOTIFY expandedLabelColorChanged
                   BINDABLE bindableExpandedLabelColor FINAL)
    Q_PROPERTY(QColor selectedIndicatorColor READ selectedIndicatorColor NOTIFY
                   selectedIndicatorColorChanged BINDABLE bindableSelectedIndicatorColor FINAL)
public:
    explicit RailItemState(QObject* parent = nullptr);
    SelectionButton*  item() const;
    void              setItem(SelectionButton*);
    QColor            collapsedLabelColor() const;
    QColor            expandedLabelColor() const;
    QBindable<QColor> bindableCollapsedLabelColor() const;
    QBindable<QColor> bindableExpandedLabelColor() const;
    QColor            selectedIndicatorColor() const;
    QBindable<QColor> bindableSelectedIndicatorColor() const;
    Q_SIGNAL void     collapsedLabelColorChanged();
    Q_SIGNAL void     expandedLabelColorChanged();
    Q_SIGNAL void     selectedIndicatorColorChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(RailItemState, QColor, m_collapsedLabelColor,
                               &RailItemState::collapsedLabelColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(RailItemState, QColor, m_expandedLabelColor,
                               &RailItemState::expandedLabelColorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(RailItemState, QColor, m_selectedIndicatorColor,
                               &RailItemState::selectedIndicatorColorChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
