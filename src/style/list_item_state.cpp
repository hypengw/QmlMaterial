#include "qml_material/style/list_item_state.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
ListItemState::ListItemState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Ignored, PressSource::Pressed) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    base.bind(m_appearance.textColor, [this] {
        return ctx() ? ctx()->textColor() : QColor(Qt::transparent);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return ctx() ? ctx()->backgroundColor() : QColor(Qt::transparent);
    });
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface_variant));
    base.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    stateBindings(Interaction::Pressed).bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        return stateTokens().pressed.state_layer_opacity;
    });
    stateBindings(Interaction::Hovered).bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        return stateTokens().hover.state_layer_opacity;
    });
}
ItemDelegate* ListItemState::item() const { return static_cast<ItemDelegate*>(inputItem()); }
void          ListItemState::setItem(ItemDelegate* item) { setInputItem(item); }
} // namespace qml_material
