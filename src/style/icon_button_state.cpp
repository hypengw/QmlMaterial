#include "qml_material/style/icon_button_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/token/token.hpp"
#include <QPointer>
namespace qml_material
{
using ButtonType = Enum::IconButtonType;
using ButtonSize = Enum::ButtonSize;

IconButtonState::IconButtonState(QObject* parent): ButtonInteractionState(parent) { bindTargets(); }

Button* IconButtonState::item() const { return static_cast<Button*>(inputItem()); }
void    IconButtonState::setItem(Button* item) { setInputItem(item); }

#define INPUT(Type, Name, Setter, Bindable)                                    \
    Type            IconButtonState::Name() const { return m_##Name.value(); } \
    void            IconButtonState::Setter(Type value) { m_##Name = value; }  \
    QBindable<Type> IconButtonState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(int, widthMode, setWidthMode, bindableWidthMode)
INPUT(int, type, setType, bindableType)
INPUT(int, size, setSize, bindableSize)
INPUT(bool, isRound, setIsRound, bindableIsRound)
#undef INPUT
#define INPUT(Type, Name, Setter, Bindable)                                          \
    Type            IconButtonState::Name() const { return m_##Name.value(); }       \
    void            IconButtonState::Setter(const Type& value) { m_##Name = value; } \
    QBindable<Type> IconButtonState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(token::IconButtonSize, sizeTokens, setSizeTokens, bindableSizeTokens)
#undef INPUT

void IconButtonState::bindTargets() {
    m_selectedSize.setBinding([this] {
        const auto sizes = m_sizeTokens.value();
        switch (ButtonSize(m_size.value())) {
        case ButtonSize::XS: return sizes.xsmall;
        case ButtonSize::M: return sizes.medium;
        case ButtonSize::L: return sizes.large;
        case ButtonSize::XL: return sizes.xlarge;
        default: return sizes.small;
        }
    });
    m_containerWidth.setBinding([this]() -> qreal {
        const auto size = m_selectedSize.value();
        switch (Enum::ButtonWidthMode(m_widthMode.value())) {
        case Enum::ButtonWidthMode::NarrowWidth: return size.narrow_width;
        case Enum::ButtonWidthMode::WideWidth: return size.wide_width;
        default: return size.default_width;
        }
    });
    const auto base = baseBindings();
    base.bind(m_appearance.textColor, [this] {
        return defaultTextColor();
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return defaultBackgroundColor();
    });
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level1;
    });
    base.bind(m_appearance.corner, [this]() -> qreal {
        const auto size = sizeToken();
        if (down()) return size.pressed_corner_size;
        if (checked() || ! isRound()) return size.corner_size;
        return backgroundHeight().value_or(40) / 2;
    });
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, [this]() -> QColor {
        const auto t = ButtonType(type());
        return t == ButtonType::IBtStandard || t == ButtonType::IBtOutlined
                   ? QColor(Qt::transparent)
                   : color(&MdColorMgr::on_surface);
    });
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered }) {
        auto active = stateBindings(state);
        active.bind(m_appearance.elevation, [this, state]() -> qreal {
            return state == Interaction::Hovered ? elevationTokens().level2
                                                 : elevationTokens().level1;
        });
        active.bind(m_appearance.stateLayerColor, [this, state] {
            if (state == Interaction::Pressed && ButtonType(type()) == ButtonType::IBtOutlined &&
                ! checked())
                return color(&MdColorMgr::on_surface);
            return defaultTextColor();
        });
    }
}

QColor IconButtonState::defaultTextColor() const {
    const bool filled = checked() || ! checkable();
    switch (ButtonType(type())) {
    case ButtonType::IBtStandard:
        return color(checked() ? &MdColorMgr::primary : &MdColorMgr::on_surface_variant);
    case ButtonType::IBtOutlined:
        return color(checked() ? &MdColorMgr::inverse_on_surface : &MdColorMgr::on_surface_variant);
    case ButtonType::IBtFilledTonal:
        return color(filled ? &MdColorMgr::on_secondary_container
                            : &MdColorMgr::on_surface_variant);
    default: return color(filled ? &MdColorMgr::on_primary : &MdColorMgr::primary);
    }
}
QColor IconButtonState::defaultBackgroundColor() const {
    const bool filled = checked() || ! checkable();
    switch (ButtonType(type())) {
    case ButtonType::IBtStandard: return Qt::transparent;
    case ButtonType::IBtOutlined:
        return checked() ? color(&MdColorMgr::inverse_surface) : QColor(Qt::transparent);
    case ButtonType::IBtFilledTonal:
        return color(filled ? &MdColorMgr::secondary_container
                            : &MdColorMgr::surface_container_highest);
    default: return color(filled ? &MdColorMgr::primary : &MdColorMgr::surface_container_highest);
    }
}

token::IconButtonSizeItem IconButtonState::sizeToken() const { return m_selectedSize.value(); }
qreal IconButtonState::containerHeight() const { return sizeToken().container_height; }
qreal IconButtonState::containerWidth() const { return m_containerWidth.value(); }
qreal IconButtonState::iconSize() const { return sizeToken().icon_size; }
} // namespace qml_material
