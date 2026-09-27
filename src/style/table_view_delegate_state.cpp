#include "qml_material/style/table_view_delegate_state.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
TableViewDelegateState::TableViewDelegateState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Separate, PressSource::Pressed) {
    m_selected.setBinding([this] {
        return itemSelected() || (item() && item()->isHighlighted());
    });
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, [this] {
        return color(selected() ? &MdColorMgr::surface_container_highest : &MdColorMgr::surface);
    });
    base.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::outline_variant));
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    bindStateLayerOpacity();
    setHoverBinding(Qt::makePropertyBinding([this] {
        return rowHovered();
    }));
}
TableViewDelegateState::~TableViewDelegateState() { stopBindings(); }
ItemDelegate* TableViewDelegateState::item() const {
    return static_cast<ItemDelegate*>(inputItem());
}
void            TableViewDelegateState::setItem(ItemDelegate* item) { setInputItem(item); }
bool            TableViewDelegateState::itemSelected() const { return m_itemSelected.value(); }
void            TableViewDelegateState::setItemSelected(bool value) { m_itemSelected = value; }
QBindable<bool> TableViewDelegateState::bindableItemSelected() {
    return QBindable<bool>(&m_itemSelected);
}
bool            TableViewDelegateState::rowHovered() const { return m_rowHovered.value(); }
void            TableViewDelegateState::setRowHovered(bool value) { m_rowHovered = value; }
QBindable<bool> TableViewDelegateState::bindableRowHovered() {
    return QBindable<bool>(&m_rowHovered);
}
bool            TableViewDelegateState::selected() const { return m_selected.value(); }
QBindable<bool> TableViewDelegateState::bindableSelected() const {
    return QBindable<bool>(&m_selected);
}
} // namespace qml_material
