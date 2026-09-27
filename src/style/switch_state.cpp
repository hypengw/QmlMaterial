#include "qml_material/style/switch_state.hpp"
#include "qml_material/control/switch.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
SwitchState::SwitchState(QObject* parent): ButtonInteractionState(parent, FocusTreatment::Ignored) {
    m_hasIcon.setBinding([this] {
        const auto* control = item();
        return control && ! control->icon()->isEmpty();
    });
    m_handleColorKey = bindingSet().property<&SwitchState::bindableHandleColor>(this);
    m_handleSizeKey  = bindingSet().property<&SwitchState::bindableHandleSize>(this);
    auto base        = baseBindings();
    auto text        = [this] {
        return color(checked() ? &MdColorMgr::on_primary_container
                               : &MdColorMgr::surface_container_highest);
    };
    auto background = [this] {
        return color(checked() ? &MdColorMgr::primary : &MdColorMgr::surface_container_highest);
    };
    base.bind(m_appearance.textColor, text);
    base.bind(m_appearance.backgroundColor, background);
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level1;
    });
    base.bind(m_handleColorKey, [this] {
        return color(checked() ? &MdColorMgr::on_primary : &MdColorMgr::outline);
    });
    base.bind(m_handleSizeKey, [this] {
        return checked() || hasIcon() ? 24 : 16;
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.textColor, text);
        active.bind(m_appearance.backgroundColor, background);
        active.bind(m_handleColorKey, [this] {
            return color(checked() ? &MdColorMgr::primary_container
                                   : &MdColorMgr::on_surface_variant);
        });
        active.bind(m_appearance.stateLayerColor, [this] {
            return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerOpacity, [this, state]() -> qreal {
            return state == Interaction::Pressed ? stateTokens().pressed.state_layer_opacity
                                                 : stateTokens().hover.state_layer_opacity;
        });
    }
    stateBindings(Interaction::Pressed).bind(m_handleSizeKey, [] {
        return 28;
    });
}
Switch*           SwitchState::item() const { return static_cast<Switch*>(inputItem()); }
void              SwitchState::setItem(Switch* item) { setInputItem(item); }
QColor            SwitchState::handleColor() const { return m_handleColor.value(); }
void              SwitchState::setHandleColor(const QColor& value) { m_handleColor = value; }
QBindable<QColor> SwitchState::bindableHandleColor() { return QBindable<QColor>(&m_handleColor); }
void              SwitchState::resetHandleColor() { m_handleColorKey.reset(); }
int               SwitchState::handleSize() const { return m_handleSize.value(); }
void              SwitchState::setHandleSize(int value) { m_handleSize = value; }
QBindable<int>    SwitchState::bindableHandleSize() { return QBindable<int>(&m_handleSize); }
void              SwitchState::resetHandleSize() { m_handleSizeKey.reset(); }
bool              SwitchState::hasIcon() const { return m_hasIcon.value(); }
} // namespace qml_material
