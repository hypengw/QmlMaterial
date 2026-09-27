#include "qml_material/style/button_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/token/token.hpp"
#include <QPointer>
namespace qml_material
{
using ButtonType = Enum::ButtonType;
using ButtonSize = Enum::ButtonSize;

ButtonAppearanceState::ButtonAppearanceState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Separate) {
    bindAppearance();
}
int            ButtonAppearanceState::type() const { return m_type.value(); }
void           ButtonAppearanceState::setType(int value) { m_type = value; }
QBindable<int> ButtonAppearanceState::bindableType() { return QBindable<int>(&m_type); }

ButtonState::ButtonState(QObject* parent): ButtonAppearanceState(parent) {
    connect(this, &ButtonState::sizeTokenChanged, this, &ButtonState::geometryChanged);
    bindTargets();
}

Button* ButtonAppearanceState::item() const { return static_cast<Button*>(inputItem()); }
void    ButtonAppearanceState::setItem(Button* item) { setInputItem(item); }

#define INPUT(Type, Name, Setter, Bindable)                                          \
    Type            ButtonAppearanceState::Name() const { return m_##Name.value(); } \
    void            ButtonAppearanceState::Setter(Type value) { m_##Name = value; }  \
    QBindable<Type> ButtonAppearanceState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(int, size, setSize, bindableSize)
INPUT(bool, isRound, setIsRound, bindableIsRound)
#undef INPUT
#define INPUT(Type, Name, Setter, Bindable)                                      \
    Type            ButtonState::Name() const { return m_##Name.value(); }       \
    void            ButtonState::Setter(const Type& value) { m_##Name = value; } \
    QBindable<Type> ButtonState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(token::ButtonSize, sizeTokens, setSizeTokens, bindableSizeTokens)
#undef INPUT

void ButtonState::bindTargets() {
    m_selectedSize.setBinding([this] {
        const auto sizes = m_sizeTokens.value();
        switch (ButtonSize(size())) {
        case ButtonSize::XS: return sizes.xsmall;
        case ButtonSize::M: return sizes.medium;
        case ButtonSize::L: return sizes.large;
        case ButtonSize::XL: return sizes.xlarge;
        default: return sizes.small;
        }
    });
    const auto base = baseBindings();
    base.bind(m_appearance.corner, [this]() -> qreal {
        const auto size = sizeToken();
        if (down()) return size.pressed_corner_size;
        if (! isRound()) return size.corner_size;
        return backgroundHeight().value_or(size.container_height) / 2;
    });
}

void ButtonAppearanceState::bindAppearance() {
    const auto base = baseBindings();
    base.bind(m_appearance.textColor, [this] {
        return defaultTextColor();
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return defaultBackgroundColor();
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return ButtonType(type()) == ButtonType::BtElevated ? elevationTokens().level1
                                                            : elevationTokens().level0;
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, [this]() -> QColor {
        const auto t = ButtonType(type());
        if (t == ButtonType::BtText || t == ButtonType::BtOutlined) return Qt::transparent;
        return color(t == ButtonType::BtElevated ? &MdColorMgr::surface_container_low
                                                 : &MdColorMgr::on_surface);
    });
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        const auto t = ButtonType(type());
        return t == ButtonType::BtText || t == ButtonType::BtOutlined ? 1
               : t == ButtonType::BtElevated ? stateTokens().disabled_content
                                             : stateTokens().disabled_container;
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered, Interaction::Focus }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.elevation, [this, state]() -> qreal {
            const auto e        = elevationTokens();
            const bool elevated = ButtonType(type()) == ButtonType::BtElevated;
            return state == Interaction::Hovered ? (elevated ? e.level2 : e.level1)
                                                 : (elevated ? e.level1 : e.level0);
        });
        active.bind(m_appearance.stateLayerColor, [this] {
            auto* p = colors();
            if (! p) return QColor(Qt::transparent);
            const auto t = ButtonType(type());
            return t == ButtonType::BtFilled || t == ButtonType::BtFilledTonal
                       ? p->getOn(backgroundColor())
                       : p->primary();
        });
    }
}

QColor ButtonAppearanceState::defaultTextColor() const {
    switch (ButtonType(type())) {
    case ButtonType::BtText: return color(&MdColorMgr::primary);
    case ButtonType::BtFilled:
        return color(checked() || ! checkable() ? &MdColorMgr::on_primary
                                                : &MdColorMgr::on_surface_variant);
    case ButtonType::BtFilledTonal:
        return color(checked() ? &MdColorMgr::on_secondary : &MdColorMgr::on_secondary_container);
    case ButtonType::BtOutlined:
        return color(checked() ? &MdColorMgr::inverse_on_surface : &MdColorMgr::on_surface_variant);
    default: return color(checked() ? &MdColorMgr::on_primary : &MdColorMgr::primary);
    }
}
QColor ButtonAppearanceState::defaultBackgroundColor() const {
    switch (ButtonType(type())) {
    case ButtonType::BtText: return Qt::transparent;
    case ButtonType::BtFilled:
        return color(checked() || ! checkable() ? &MdColorMgr::primary
                                                : &MdColorMgr::surface_container);
    case ButtonType::BtFilledTonal:
        return color(checked() ? &MdColorMgr::secondary : &MdColorMgr::secondary_container);
    case ButtonType::BtOutlined:
        return checked() ? color(&MdColorMgr::inverse_surface) : QColor(Qt::transparent);
    default: return color(checked() ? &MdColorMgr::primary : &MdColorMgr::surface_container_low);
    }
}

token::ButtonSizeItem ButtonState::sizeToken() const { return m_selectedSize.value(); }
qreal                 ButtonState::containerHeight() const { return sizeToken().container_height; }
qreal                 ButtonState::iconSize() const { return sizeToken().icon_size; }
qreal                 ButtonState::leadingSpace() const { return sizeToken().leading_space; }
qreal                 ButtonState::trailingSpace() const { return sizeToken().trailing_space; }
qreal                 ButtonState::spacing() const { return sizeToken().spacing; }

} // namespace qml_material
