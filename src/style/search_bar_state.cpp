#include "qml_material/style/search_bar_state.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
SearchBarState::SearchBarState(QObject* parent): ButtonInteractionState(parent) {
    m_placeholderKey = bindingSet().property<&SearchBarState::bindablePlaceholderOpacity>(this);
    auto base        = baseBindings();
    base.bind(m_placeholderKey, [] {
        return qreal(1);
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::surface_container_highest));
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::outline));
    auto disabled = disabledBindings();
    disabled.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_placeholderKey, [] {
        return qreal(.38);
    });
    disabled.bind(m_appearance.backgroundOpacity, [] {
        return qreal(.12);
    });
    auto active = appearanceWhen([this] {
        return pressed() || hovered() || visualFocus();
    });
    active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    bindStateLayerOpacity();
}
ButtonControl* SearchBarState::item() const { return inputItem(); }
void           SearchBarState::setItem(ButtonControl* item) { setInputItem(item, {}, {}); }
qreal          SearchBarState::placeholderOpacity() const { return m_placeholderOpacity.value(); }
void           SearchBarState::setPlaceholderOpacity(qreal value) { m_placeholderOpacity = value; }
void           SearchBarState::resetPlaceholderOpacity() { m_placeholderKey.reset(); }
QBindable<qreal> SearchBarState::bindablePlaceholderOpacity() {
    return QBindable<qreal>(&m_placeholderOpacity);
}
} // namespace qml_material
