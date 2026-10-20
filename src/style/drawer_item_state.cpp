#include "qml_material/style/drawer_item_state.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
DrawerItemState::DrawerItemState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container
                               : &MdColorMgr::on_surface_variant);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return checked() ? color(&MdColorMgr::secondary_container) : QColor(Qt::transparent);
    });
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    auto disabled = disabledBindings();
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [] {
        return qreal(.38);
    });
    disabled.bind(m_appearance.backgroundOpacity, [] {
        return qreal(.38);
    });
    auto active = appearanceWhen([this] {
        return pressed() || hovered() || visualFocus();
    });
    active.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
    });
    active.bind(m_appearance.stateLayerColor, [this] {
        return color(pressed() || checked() ? &MdColorMgr::on_secondary_container
                                            : &MdColorMgr::on_surface);
    });
    bindStateLayerOpacity();
}
SelectionButton* DrawerItemState::item() const {
    return static_cast<SelectionButton*>(inputItem());
}
void DrawerItemState::setItem(SelectionButton* item) { setInputItem(item); }
} // namespace qml_material
