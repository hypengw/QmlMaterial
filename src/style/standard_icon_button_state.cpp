#include "qml_material/style/standard_icon_button_state.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
StandardIconButtonState::StandardIconButtonState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.textColor, [this] {
        return color(&MdColorMgr::on_surface_variant);
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    auto disabled = disabledBindings();
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
    appearanceWhen([this] {
        return pressed() || hovered() || visualFocus();
    }).bind(m_appearance.stateLayerColor, [this] {
        return color(&MdColorMgr::on_surface_variant);
    });
}
ButtonControl* StandardIconButtonState::item() const { return inputItem(); }
void           StandardIconButtonState::setItem(ButtonControl* item) { setInputItem(item, {}, {}); }

} // namespace qml_material
