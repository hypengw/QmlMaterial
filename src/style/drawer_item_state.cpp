#include "qml_material/style/drawer_item_state.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
DrawerItemState::DrawerItemState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Pressed, PressSource::Pressed) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container
                               : &MdColorMgr::on_surface_variant);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return checked() ? color(&MdColorMgr::secondary_container) : QColor(Qt::transparent);
    });
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [] {
        return qreal(.38);
    });
    disabled.bind(m_appearance.backgroundOpacity, [] {
        return qreal(.38);
    });
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.textColor, [this] {
            return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerColor, [this, state] {
            return color(state == Interaction::Pressed || checked()
                             ? &MdColorMgr::on_secondary_container
                             : &MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerOpacity, [this, state]() -> qreal {
            return state == Interaction::Pressed ? stateTokens().pressed.state_layer_opacity
                                                 : stateTokens().hover.state_layer_opacity;
        });
    }
}
ItemDelegate* DrawerItemState::item() const { return static_cast<ItemDelegate*>(inputItem()); }
void          DrawerItemState::setItem(ItemDelegate* item) { setInputItem(item); }
} // namespace qml_material
