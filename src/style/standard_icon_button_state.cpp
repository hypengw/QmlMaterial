#include "qml_material/style/standard_icon_button_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
StandardIconButtonState::StandardIconButtonState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface_variant);
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered })
        stateBindings(state).bind(m_appearance.stateLayerColor, [this] {
            return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface_variant);
        });
}
Button* StandardIconButtonState::item() const { return static_cast<Button*>(inputItem()); }
void    StandardIconButtonState::setItem(Button* item) { setInputItem(item); }

} // namespace qml_material
