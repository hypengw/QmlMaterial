#include "qml_material/style/rail_item_state.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
RailItemState::RailItemState(QObject* parent): ButtonInteractionState(parent) {
    indicationPolicy().bindDefaultColor(
        Qt::makePropertyBinding(colorBinding(&MdColorMgr::on_surface)));
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::on_surface : &MdColorMgr::on_surface_variant);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return checked() ? selectedIndicatorColor() : QColor(Qt::transparent);
    });
    m_selectedIndicatorColor.setBinding([this] {
        return color(&MdColorMgr::secondary_container);
    });
    base.bind(m_appearance.supportTextColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container
                               : &MdColorMgr::on_surface_variant);
    });
    m_collapsedLabelColor.setBinding([this] {
        return color(checked() ? &MdColorMgr::secondary : &MdColorMgr::on_surface_variant);
    });
    m_expandedLabelColor.setBinding([this] {
        return color(checked() ? &MdColorMgr::on_secondary_container
                               : &MdColorMgr::on_surface_variant);
    });
    auto active = appearanceWhen([this] {
        return pressed() || hovered() || visualFocus();
    });
    active.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    active.bind(m_appearance.supportTextColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
    });
    active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    bindStateLayerOpacity();
}
SelectionButton* RailItemState::item() const { return static_cast<SelectionButton*>(inputItem()); }
void             RailItemState::setItem(SelectionButton* item) { setInputItem(item); }
QColor RailItemState::collapsedLabelColor() const { return m_collapsedLabelColor.value(); }
QColor RailItemState::expandedLabelColor() const { return m_expandedLabelColor.value(); }
QBindable<QColor> RailItemState::bindableCollapsedLabelColor() const {
    return QBindable<QColor>(&m_collapsedLabelColor);
}
QBindable<QColor> RailItemState::bindableExpandedLabelColor() const {
    return QBindable<QColor>(&m_expandedLabelColor);
}
QColor RailItemState::selectedIndicatorColor() const { return m_selectedIndicatorColor.value(); }
QBindable<QColor> RailItemState::bindableSelectedIndicatorColor() const {
    return QBindable<QColor>(&m_selectedIndicatorColor);
}
} // namespace qml_material
