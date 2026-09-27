#include "qml_material/style/chip_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
ChipState::ChipState(Kind kind, QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Separate), m_kind(kind) {
    auto base = baseBindings();
    base.bind(m_appearance.textColor, [this] {
        return defaultTextColor();
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return selected()   ? color(&MdColorMgr::secondary_container)
               : elevated() ? color(&MdColorMgr::surface_container_low)
                            : QColor(Qt::transparent);
    });
    base.bind(m_appearance.outlineColor, [this] {
        return color(selected() ? &MdColorMgr::secondary_container : &MdColorMgr::outline_variant);
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevated() ? elevationTokens().level1 : elevationTokens().level0;
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
    if (kind == Kind::Filter)
        disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered, Interaction::Focus }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.stateLayerColor, [this] {
            return defaultTextColor();
        });
        if (kind == Kind::Assist || kind == Kind::Suggestion)
            active.bind(m_appearance.textColor, [this] {
                return defaultTextColor();
            });
    }
    stateBindings(Interaction::Hovered).bind(m_appearance.elevation, [this]() -> qreal {
        return elevated()   ? elevationTokens().level2
               : selected() ? elevationTokens().level1
                            : elevationTokens().level0;
    });
    stateBindings(Interaction::Focus).bind(m_appearance.outlineColor, [this] {
        return selected() ? color(&MdColorMgr::secondary_container) : defaultTextColor();
    });
}
Button*         ChipState::item() const { return static_cast<Button*>(inputItem()); }
void            ChipState::setItem(Button* item) { setInputItem(item); }
bool            ChipState::elevated() const { return m_elevated.value(); }
void            ChipState::setElevated(bool value) { m_elevated = value; }
QBindable<bool> ChipState::bindableElevated() { return QBindable<bool>(&m_elevated); }

bool ChipState::selected() const {
    return (m_kind == Kind::Filter || m_kind == Kind::Input || m_kind == Kind::Embed) && checked();
}
QColor ChipState::defaultTextColor() const {
    return color(selected()               ? &MdColorMgr::on_secondary_container
                 : m_kind == Kind::Assist ? &MdColorMgr::on_surface
                                          : &MdColorMgr::on_surface_variant);
}
QColor ChipState::resolveIconColor(bool leading) const {
    return color(selected() ? &MdColorMgr::on_secondary_container
                 : leading  ? &MdColorMgr::primary
                            : &MdColorMgr::on_surface_variant);
}
SingleIconChipState::SingleIconChipState(Kind kind, QObject* parent): ChipState(kind, parent) {
    m_iconKey = bindingSet().property<&SingleIconChipState::bindableIconColor>(this);
    baseBindings().bind(m_iconKey, [this] {
        return resolveIconColor(true);
    });
    stateBindings(Interaction::Disabled).bind(m_iconKey, colorBinding(&MdColorMgr::on_surface));
}
QColor            SingleIconChipState::iconColor() const { return m_iconColor.value(); }
void              SingleIconChipState::setIconColor(const QColor& value) { m_iconColor = value; }
QBindable<QColor> SingleIconChipState::bindableIconColor() {
    return QBindable<QColor>(&m_iconColor);
}

void SingleIconChipState::resetIconColor() { m_iconKey.reset(); }
DualIconChipState::DualIconChipState(Kind kind, QObject* parent): ChipState(kind, parent) {
    m_leadingIconKey  = bindingSet().property<&DualIconChipState::bindableLeadingIconColor>(this);
    m_trailingIconKey = bindingSet().property<&DualIconChipState::bindableTrailingIconColor>(this);
    baseBindings().bind(m_leadingIconKey, [this] {
        return resolveIconColor(true);
    });
    baseBindings().bind(m_trailingIconKey, [this] {
        return resolveIconColor(false);
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_leadingIconKey, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_trailingIconKey, colorBinding(&MdColorMgr::on_surface));
}
QColor DualIconChipState::leadingIconColor() const { return m_leadingIconColor.value(); }
void   DualIconChipState::setLeadingIconColor(const QColor& value) { m_leadingIconColor = value; }
QBindable<QColor> DualIconChipState::bindableLeadingIconColor() {
    return QBindable<QColor>(&m_leadingIconColor);
}
QColor DualIconChipState::trailingIconColor() const { return m_trailingIconColor.value(); }
void   DualIconChipState::setTrailingIconColor(const QColor& value) { m_trailingIconColor = value; }
QBindable<QColor> DualIconChipState::bindableTrailingIconColor() {
    return QBindable<QColor>(&m_trailingIconColor);
}

void DualIconChipState::resetLeadingIconColor() { m_leadingIconKey.reset(); }
void DualIconChipState::resetTrailingIconColor() { m_trailingIconKey.reset(); }
AssistChipState::AssistChipState(QObject* parent): SingleIconChipState(Kind::Assist, parent) {}
SuggestionChipState::SuggestionChipState(QObject* parent)
    : SingleIconChipState(Kind::Suggestion, parent) {}
FilterChipState::FilterChipState(QObject* parent): DualIconChipState(Kind::Filter, parent) {}
InputChipState::InputChipState(QObject* parent): DualIconChipState(Kind::Input, parent) {}
EmbedChipState::EmbedChipState(QObject* parent): DualIconChipState(Kind::Embed, parent) {}
int            EmbedChipState::borderWidth() const { return m_borderWidth.value(); }
void           EmbedChipState::setBorderWidth(int value) { m_borderWidth = value; }
QBindable<int> EmbedChipState::bindableBorderWidth() { return QBindable<int>(&m_borderWidth); }
} // namespace qml_material
