#include "qml_material/style/bar_item_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
BarItemState::BarItemState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
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
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
        active.bind(m_appearance.supportTextColor, [this] {
            return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    }
}
Button* BarItemState::item() const { return static_cast<Button*>(inputItem()); }
void    BarItemState::setItem(Button* item) { setInputItem(item); }

} // namespace qml_material
