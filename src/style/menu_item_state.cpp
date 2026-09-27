#include "qml_material/style/menu_item_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
MenuItemState::MenuItemState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Separate) {
    m_leadingKey  = bindingSet().property<&MenuItemState::bindableLeadingIconColor>(this);
    m_trailingKey = bindingSet().property<&MenuItemState::bindableTrailingIconColor>(this);
    auto base     = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level2;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, [this] {
        return selected() ? color(&MdColorMgr::secondary_container) : QColor(Qt::transparent);
    });
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_leadingKey, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_trailingKey, colorBinding(&MdColorMgr::on_surface));
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.contentOpacity, [] {
        return qreal(.38);
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered, Interaction::Focus }) {
        auto active = stateBindings(state);
        active.bind(m_leadingKey, colorBinding(&MdColorMgr::on_surface_variant));
        active.bind(m_trailingKey, colorBinding(&MdColorMgr::on_surface_variant));
        active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    }
}
Button*         MenuItemState::item() const { return static_cast<Button*>(inputItem()); }
void            MenuItemState::setItem(Button* item) { setInputItem(item); }
bool            MenuItemState::selected() const { return m_selected.value(); }
void            MenuItemState::setSelected(bool value) { m_selected = value; }
QBindable<bool> MenuItemState::bindableSelected() { return QBindable<bool>(&m_selected); }
QColor          MenuItemState::leadingIconColor() const { return m_leadingIconColor.value(); }
void MenuItemState::setLeadingIconColor(const QColor& value) { m_leadingIconColor = value; }
void MenuItemState::resetLeadingIconColor() { m_leadingKey.reset(); }
QBindable<QColor> MenuItemState::bindableLeadingIconColor() {
    return QBindable<QColor>(&m_leadingIconColor);
}
QColor MenuItemState::trailingIconColor() const { return m_trailingIconColor.value(); }
void   MenuItemState::setTrailingIconColor(const QColor& value) { m_trailingIconColor = value; }
void   MenuItemState::resetTrailingIconColor() { m_trailingKey.reset(); }
QBindable<QColor> MenuItemState::bindableTrailingIconColor() {
    return QBindable<QColor>(&m_trailingIconColor);
}
} // namespace qml_material
