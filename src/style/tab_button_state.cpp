#include "qml_material/style/tab_button_state.hpp"
#include "qml_material/control/tab_button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
TabButtonState::TabButtonState(QObject* parent): ButtonInteractionState(parent) {
    resetBaseTextColor();
    auto base = baseBindings();
    base.bind(m_appearance.textColor, [this] {
        return colors() ? baseTextColor() : QColor(Qt::transparent);
    });
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::surface));
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.textColor, [this] {
            if (! colors()) return QColor(Qt::transparent);
            return type() == int(Enum::TabType::PrimaryTab) ? baseTextColor()
                                                            : color(&MdColorMgr::on_surface);
        });
        active.bind(m_appearance.stateLayerColor, [this] {
            return color(type() == int(Enum::TabType::PrimaryTab) ? &MdColorMgr::primary
                                                                  : &MdColorMgr::on_surface);
        });
    }
}
TabButton*        TabButtonState::item() const { return static_cast<TabButton*>(inputItem()); }
void              TabButtonState::setItem(TabButton* item) { setInputItem(item); }
int               TabButtonState::type() const { return m_type.value(); }
void              TabButtonState::setType(int type) { m_type = type; }
QBindable<int>    TabButtonState::bindableType() { return QBindable<int>(&m_type); }
QColor            TabButtonState::baseTextColor() const { return m_baseTextColor.value(); }
void              TabButtonState::setBaseTextColor(const QColor& color) { m_baseTextColor = color; }
QBindable<QColor> TabButtonState::bindableBaseTextColor() {
    return QBindable<QColor>(&m_baseTextColor);
}
void TabButtonState::resetBaseTextColor() {
    m_baseTextColor.setBinding([this] {
        const auto palette = colors();
        if (! palette) return QColor(Qt::transparent);
        if (type() == int(Enum::TabType::PrimaryTab))
            return checked() ? palette->primary() : palette->on_surface();
        return checked() ? palette->on_surface() : palette->on_surface_variant();
    });
}

} // namespace qml_material
