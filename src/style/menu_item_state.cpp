#include "qml_material/style/menu_item_state.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
MenuItemState::MenuItemState(QObject* parent): ButtonInteractionState(parent) {
    m_leadingKey  = bindingSet().property<&MenuItemState::bindableLeadingIconColor>(this);
    m_trailingKey = bindingSet().property<&MenuItemState::bindableTrailingIconColor>(this);
    auto base     = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level2;
    });
    const auto foreground = [this](auto standard) {
        if (! expressive()) return color(standard);
        if (vibrant()) {
            return selected() ? color(&MdColorMgr::on_tertiary)
                              : color(&MdColorMgr::on_tertiary_container);
        }
        return selected() ? color(&MdColorMgr::on_tertiary_container) : color(standard);
    };
    base.bind(m_appearance.textColor, [foreground] {
        return foreground(&MdColorMgr::on_surface);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        if (! selected()) return QColor(Qt::transparent);
        if (! expressive()) return color(&MdColorMgr::surface_container_highest);
        return vibrant() ? color(&MdColorMgr::tertiary) : color(&MdColorMgr::tertiary_container);
    });
    auto supporting = [foreground] {
        return foreground(&MdColorMgr::on_surface_variant);
    };
    base.bind(m_appearance.supportTextColor, supporting);
    base.bind(m_leadingKey, supporting);
    base.bind(m_trailingKey, supporting);
    base.bind(m_appearance.stateLayerColor, [foreground] {
        return foreground(&MdColorMgr::on_surface);
    });
    auto disabled = disabledBindings();
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.contentOpacity, [] {
        return qreal(.38);
    });
    bindStateLayerOpacity();
}
ButtonControl*  MenuItemState::item() const { return inputItem(); }
void            MenuItemState::setItem(ButtonControl* item) { setInputItem(item, {}, {}); }
bool            MenuItemState::selected() const { return m_selected.value(); }
void            MenuItemState::setSelected(bool value) { m_selected = value; }
QBindable<bool> MenuItemState::bindableSelected() { return QBindable<bool>(&m_selected); }
QColor          MenuItemState::leadingIconColor() const { return m_leadingIconColor.value(); }
void MenuItemState::setLeadingIconColor(const QColor& value) { m_leadingIconColor = value; }
void MenuItemState::resetLeadingIconColor() { m_leadingKey.reset(); }
QBindable<QColor> MenuItemState::bindableLeadingIconColor() {
    return QBindable<QColor>(&m_leadingIconColor);
}
QColor MenuItemState::trailingIconColor() const { return m_trailingIconColor.value(); }
void   MenuItemState::setTrailingIconColor(const QColor& value) { m_trailingIconColor = value; }
void   MenuItemState::resetTrailingIconColor() { m_trailingKey.reset(); }
QBindable<QColor> MenuItemState::bindableTrailingIconColor() {
    return QBindable<QColor>(&m_trailingIconColor);
}
} // namespace qml_material
