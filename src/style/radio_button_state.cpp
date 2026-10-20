#include "qml_material/style/radio_button_state.hpp"
#include "qml_material/control/radio_button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
RadioButtonState::RadioButtonState(QObject* parent): ButtonInteractionState(parent) {
    indicationPolicy().bindDefaultColor(Qt::makePropertyBinding([this] {
        return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface);
    }));
    m_iconKey = bindingSet().property<&RadioButtonState::bindableIconColor>(this);
    baseBindings().bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    baseBindings().bind(m_iconKey, [this] {
        return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface_variant);
    });
    auto disabled = disabledBindings();
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_iconKey, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    bindStateLayerOpacity();
    appearanceWhen([this] {
        return pressed() || hovered() || visualFocus();
    }).bind(m_iconKey, [this] {
        return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface);
    });
    appearanceWhen([this] {
        return hovered() || visualFocus();
    }).bind(m_appearance.stateLayerColor, [this] {
        return iconColor();
    });
    appearanceWhen([this] {
        return pressed();
    }).bind(m_appearance.stateLayerColor, [this] {
        return color(checked() ? &MdColorMgr::on_surface : &MdColorMgr::primary);
    });
}
RadioButton*      RadioButtonState::item() const { return static_cast<RadioButton*>(inputItem()); }
void              RadioButtonState::setItem(RadioButton* item) { setInputItem(item); }
QColor            RadioButtonState::iconColor() const { return m_iconColor.value(); }
void              RadioButtonState::setIconColor(const QColor& value) { m_iconColor = value; }
QBindable<QColor> RadioButtonState::bindableIconColor() { return QBindable<QColor>(&m_iconColor); }

void RadioButtonState::resetIconColor() { m_iconKey.reset(); }
} // namespace qml_material
