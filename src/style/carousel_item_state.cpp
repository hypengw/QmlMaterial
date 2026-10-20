#include "qml_material/style/carousel_item_state.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
CarouselItemState::CarouselItemState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level1;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::surface_container_low));
    base.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
}
CarouselItemState::~CarouselItemState() { stopBindings(); }
ButtonControl* CarouselItemState::item() const { return inputItem(); }
void           CarouselItemState::setItem(ButtonControl* item) { setInputItem(item, {}, {}); }
} // namespace qml_material
