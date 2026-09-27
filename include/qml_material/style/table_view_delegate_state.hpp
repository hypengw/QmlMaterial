#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/item_delegate.hpp")
namespace qml_material
{
class ItemDelegate;
class QML_MATERIAL_API TableViewDelegateState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateTableViewDelegate)
    Q_PROPERTY(qml_material::ItemDelegate* item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(bool itemSelected READ itemSelected WRITE setItemSelected NOTIFY itemSelectedChanged
                   BINDABLE bindableItemSelected FINAL)
    Q_PROPERTY(bool rowHovered READ rowHovered WRITE setRowHovered NOTIFY rowHoveredChanged BINDABLE
                   bindableRowHovered FINAL)
    Q_PROPERTY(bool selected READ selected NOTIFY selectedChanged BINDABLE bindableSelected FINAL)
public:
    explicit TableViewDelegateState(QObject* parent = nullptr);
    ~TableViewDelegateState() override;
    ItemDelegate*   item() const;
    void            setItem(ItemDelegate*);
    bool            itemSelected() const;
    void            setItemSelected(bool);
    QBindable<bool> bindableItemSelected();
    bool            rowHovered() const;
    void            setRowHovered(bool);
    QBindable<bool> bindableRowHovered();
    bool            selected() const;
    QBindable<bool> bindableSelected() const;
    Q_SIGNAL void   itemSelectedChanged();
    Q_SIGNAL void   rowHoveredChanged();
    Q_SIGNAL void   selectedChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TableViewDelegateState, bool, m_itemSelected, false,
                                         &TableViewDelegateState::itemSelectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TableViewDelegateState, bool, m_rowHovered, false,
                                         &TableViewDelegateState::rowHoveredChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TableViewDelegateState, bool, m_selected, false,
                                         &TableViewDelegateState::selectedChanged)
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
