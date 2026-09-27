#include "qml_material/style/rail_item_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
RailItemState::RailItemState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::on_surface : &MdColorMgr::on_surface_variant);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return checked() ? color(&MdColorMgr::secondary_container) : QColor(Qt::transparent);
    });
    base.bind(m_appearance.supportTextColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container
                               : &MdColorMgr::on_surface_variant);
    });
    m_collapsedLabelColor.setBinding([this] {
        return color(checked() ? &MdColorMgr::secondary : &MdColorMgr::on_surface_variant);
    });
    m_expandedLabelColor.setBinding([this] {
        return color(checked() ? &MdColorMgr::on_secondary_container
                               : &MdColorMgr::on_surface_variant);
    });
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
        active.bind(m_appearance.supportTextColor, [this] {
            return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
        active.bind(m_appearance.stateLayerOpacity, [this, state]() -> qreal {
            return state == Interaction::Pressed ? stateTokens().pressed.state_layer_opacity
                                                 : stateTokens().hover.state_layer_opacity;
        });
    }
}
Button* RailItemState::item() const { return static_cast<Button*>(inputItem()); }
void    RailItemState::setItem(Button* item) { setInputItem(item); }
QColor  RailItemState::collapsedLabelColor() const { return m_collapsedLabelColor.value(); }
QColor  RailItemState::expandedLabelColor() const { return m_expandedLabelColor.value(); }
QBindable<QColor> RailItemState::bindableCollapsedLabelColor() const {
    return QBindable<QColor>(&m_collapsedLabelColor);
}
QBindable<QColor> RailItemState::bindableExpandedLabelColor() const {
    return QBindable<QColor>(&m_expandedLabelColor);
}
} // namespace qml_material
