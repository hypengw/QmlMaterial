#pragma once
#include "qml_material/style/button_interaction_state.hpp"
Q_MOC_INCLUDE("qml_material/control/item_delegate.hpp")
namespace qml_material
{
class ItemDelegate;
class QML_MATERIAL_API ListItemState : public ButtonInteractionState {
    Q_OBJECT
    QML_NAMED_ELEMENT(StateListItem)
    Q_PROPERTY(qml_material::ItemDelegate* item READ item WRITE setItem NOTIFY itemChanged FINAL)
public:
    explicit ListItemState(QObject* parent = nullptr);
    ItemDelegate* item() const;
    void          setItem(ItemDelegate*);

private:
    BindingLifetime m_bindingLifetime { bindingSet().lifetime() };
};
} // namespace qml_material
