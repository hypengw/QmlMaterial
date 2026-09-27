#include "qml_material/style/card_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
CardState::CardState(QObject* parent): ButtonInteractionState(parent) {
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this] {
        return baseElevation();
    });
    base.bind(m_appearance.backgroundColor, [this] {
        switch (Enum::CardType(type())) {
        case Enum::CardType::CardOutlined: return color(&MdColorMgr::surface);
        case Enum::CardType::CardFilled: return color(&MdColorMgr::surface_container_highest);
        default: return color(&MdColorMgr::surface_container_low);
        }
    });
    base.bind(m_appearance.textColor, [this] {
        auto* palette = colors();
        return palette ? palette->getOn(backgroundColor()) : QColor(Qt::transparent);
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return Enum::CardType(type()) == Enum::CardType::CardFilled ? elevationTokens().level1
                                                                    : elevationTokens().level0;
    });
    disabled.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::surface_variant));
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
    stateBindings(Interaction::Pressed).bind(m_appearance.elevation, [this] {
        return baseElevation();
    });
    stateBindings(Interaction::Hovered).bind(m_appearance.elevation, [this]() -> qreal {
        const auto t = Enum::CardType(type());
        return t == Enum::CardType::CardOutlined || t == Enum::CardType::CardFilled
                   ? elevationTokens().level1
                   : elevationTokens().level2;
    });
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::on_surface));
        active.bind(m_appearance.stateLayerOpacity, [this, state]() -> qreal {
            return state == Interaction::Pressed ? stateTokens().pressed.state_layer_opacity
                                                 : stateTokens().hover.state_layer_opacity;
        });
    }
}
qreal CardState::baseElevation() const {
    const auto t = Enum::CardType(type());
    return t == Enum::CardType::CardOutlined || t == Enum::CardType::CardFilled
               ? elevationTokens().level0
               : elevationTokens().level1;
}
Button*        CardState::item() const { return static_cast<Button*>(inputItem()); }
void           CardState::setItem(Button* item) { setInputItem(item); }
int            CardState::type() const { return m_type.value(); }
void           CardState::setType(int value) { m_type = value; }
QBindable<int> CardState::bindableType() { return QBindable<int>(&m_type); }
int            CardState::radius() const { return m_radius.value(); }
void           CardState::setRadius(int value) { m_radius = value; }
QBindable<int> CardState::bindableRadius() { return QBindable<int>(&m_radius); }
} // namespace qml_material
