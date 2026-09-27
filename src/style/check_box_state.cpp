#include "qml_material/style/check_box_state.hpp"
#include "qml_material/control/check_box.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
CheckBoxState::CheckBoxState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Separate, PressSource::Pressed) {
    m_iconKey           = bindingSet().property<&CheckBoxState::bindableIconColor>(this);
    m_iconBackgroundKey = bindingSet().property<&CheckBoxState::bindableIconBackgroundColor>(this);
    const auto base     = baseBindings();
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.outlineColor, [this] {
        return color(error() ? &MdColorMgr::error : &MdColorMgr::on_surface_variant);
    });
    base.bind(m_iconKey, [this] {
        return color(error() ? &MdColorMgr::on_error : &MdColorMgr::on_primary);
    });
    base.bind(m_iconBackgroundKey, [this] {
        return color(error() ? &MdColorMgr::error : &MdColorMgr::primary);
    });
    const auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_iconKey, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_iconBackgroundKey, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered, Interaction::Focus }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.outlineColor, [this] {
            return color(error() ? &MdColorMgr::error : &MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerColor, [this] {
            return color(error()     ? &MdColorMgr::error
                         : checked() ? &MdColorMgr::primary
                                     : &MdColorMgr::on_surface);
        });
    }
}
CheckBox*         CheckBoxState::item() const { return static_cast<CheckBox*>(inputItem()); }
void              CheckBoxState::setItem(CheckBox* item) { setInputItem(item); }
bool              CheckBoxState::error() const { return m_error.value(); }
void              CheckBoxState::setError(bool value) { m_error = value; }
QBindable<bool>   CheckBoxState::bindableError() { return QBindable<bool>(&m_error); }
QColor            CheckBoxState::iconColor() const { return m_iconColor.value(); }
void              CheckBoxState::setIconColor(const QColor& value) { m_iconColor = value; }
QBindable<QColor> CheckBoxState::bindableIconColor() { return QBindable<QColor>(&m_iconColor); }
QColor CheckBoxState::iconBackgroundColor() const { return m_iconBackgroundColor.value(); }
void   CheckBoxState::setIconBackgroundColor(const QColor& value) { m_iconBackgroundColor = value; }
QBindable<QColor> CheckBoxState::bindableIconBackgroundColor() {
    return QBindable<QColor>(&m_iconBackgroundColor);
}

void CheckBoxState::resetIconColor() { m_iconKey.reset(); }
void CheckBoxState::resetIconBackgroundColor() { m_iconBackgroundKey.reset(); }
} // namespace qml_material
