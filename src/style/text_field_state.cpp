#include "qml_material/style/text_field_state.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
TextFieldState::TextFieldState(QObject* parent): InputState(parent) {
    m_hovered.setBinding([this] {
        return item() && item()->hovered();
    });
    connect(this, &InputState::itemChanged, this, &TextFieldState::updateNativeInputs);
    m_selectedSize.setBinding([this] {
        const auto tokens = sizeTokens();
        switch (Enum::ButtonSize(size())) {
        case Enum::ButtonSize::XS: return tokens.xsmall;
        case Enum::ButtonSize::S: return tokens.small;
        case Enum::ButtonSize::L: return tokens.large;
        case Enum::ButtonSize::XL: return tokens.xlarge;
        default: return tokens.medium;
        }
    });
    m_typescaleKey        = m_bindings.property<&TextFieldState::bindableTypescale>(this);
    m_indicatorHeightKey  = m_bindings.property<&TextFieldState::bindableIndicatorHeight>(this);
    m_indicatorColorKey   = m_bindings.property<&TextFieldState::bindableIndicatorColor>(this);
    m_placeholderColorKey = m_bindings.property<&TextFieldState::bindablePlaceholderColor>(this);
    m_placeholderOpacityKey =
        m_bindings.property<&TextFieldState::bindablePlaceholderOpacity>(this);
    bindInputAppearance(m_placeholderColorKey, m_placeholderOpacityKey);
    auto base = m_bindings.base();
    base.bind(m_typescaleKey, [this] {
        return sizeToken().type_scale;
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return type() == int(Enum::TextFieldType::TextFieldFilled)
                   ? color(&MdColorMgr::surface_container_highest)
                   : QColor(Qt::transparent);
    });
    base.bind(m_appearance.corner, [] {
        return qreal(token::Shape {}.corner.extra_small);
    });
    base.bind(m_indicatorHeightKey, [] {
        return 1;
    });
    base.bind(m_indicatorColorKey, colorBinding(&MdColorMgr::on_surface_variant));
    for (auto state : { Interaction::Error, Interaction::ErrorHover }) {
        auto error = m_bindings.state(state);
        error.bind(m_indicatorColorKey,
                   colorBinding(state == Interaction::Error ? &MdColorMgr::error
                                                            : &MdColorMgr::on_error_container));
        error.bind(m_indicatorHeightKey, [this] {
            return m_focused.value() ? 2 : 1;
        });
    }
    m_bindings.state(Interaction::Focus).bind(m_indicatorHeightKey, [] {
        return 2;
    });
    for (auto state : { Interaction::Focus, Interaction::Hovered }) {
        auto active = m_bindings.state(state);
        active.bind(m_indicatorColorKey,
                    colorBinding(state == Interaction::Focus ? &MdColorMgr::primary
                                                             : &MdColorMgr::on_surface));
        active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::primary));
        active.bind(m_appearance.stateLayerOpacity, [this, state]() -> qreal {
            return state == Interaction::Focus ? stateTokens().pressed.state_layer_opacity
                                               : stateTokens().hover.state_layer_opacity;
        });
    }
}
TextFieldState::~TextFieldState() {
    m_bindings.abandon();
    m_hovered.takeBinding();
    m_focused.takeBinding();
    m_acceptable.takeBinding();
    utils::disconnectAll(m_nativeConnections);
}
void TextFieldState::updateNativeInputs() {
    utils::disconnectAll(m_nativeConnections);
    if (auto* control = item()) {
        m_nativeConnections.append(connect(control, &QQuickItem::focusChanged, this, [this] {
            updateStateInput(m_focused, item() && item()->hasFocus());
        }));
        m_nativeConnections.append(
            connect(control, &QQuickTextInput::acceptableInputChanged, this, [this] {
                updateStateInput(m_acceptable, ! item() || item()->hasAcceptableInput());
            }));
    }
    const QScopedPropertyUpdateGroup group;
    updateStateInput(m_focused, item() && item()->hasFocus());
    updateStateInput(m_acceptable, ! item() || item()->hasAcceptableInput());
}
TextField*           TextFieldState::item() const { return static_cast<TextField*>(inputItem()); }
void                 TextFieldState::setItem(TextField* item) { setInputItem(item); }
int                  TextFieldState::size() const { return m_size.value(); }
void                 TextFieldState::setSize(const int& value) { m_size = value; }
QBindable<int>       TextFieldState::bindableSize() { return QBindable<int>(&m_size); }
int                  TextFieldState::type() const { return m_type.value(); }
void                 TextFieldState::setType(const int& value) { m_type = value; }
QBindable<int>       TextFieldState::bindableType() { return QBindable<int>(&m_type); }
token::TextFieldSize TextFieldState::sizeTokens() const { return m_sizeTokens.value(); }
void TextFieldState::setSizeTokens(const token::TextFieldSize& value) { m_sizeTokens = value; }
QBindable<token::TextFieldSize> TextFieldState::bindableSizeTokens() {
    return QBindable<token::TextFieldSize>(&m_sizeTokens);
}
token::TypeScaleItem TextFieldState::typescale() const { return m_typescale.value(); }
void TextFieldState::setTypescale(const token::TypeScaleItem& value) { m_typescale = value; }
QBindable<token::TypeScaleItem> TextFieldState::bindableTypescale() {
    return QBindable<token::TypeScaleItem>(&m_typescale);
}
void           TextFieldState::resetTypescale() { m_typescaleKey.reset(); }
int            TextFieldState::indicatorHeight() const { return m_indicatorHeight.value(); }
void           TextFieldState::setIndicatorHeight(const int& value) { m_indicatorHeight = value; }
QBindable<int> TextFieldState::bindableIndicatorHeight() {
    return QBindable<int>(&m_indicatorHeight);
}
void   TextFieldState::resetIndicatorHeight() { m_indicatorHeightKey.reset(); }
QColor TextFieldState::indicatorColor() const { return m_indicatorColor.value(); }
void   TextFieldState::setIndicatorColor(const QColor& value) { m_indicatorColor = value; }
QBindable<QColor> TextFieldState::bindableIndicatorColor() {
    return QBindable<QColor>(&m_indicatorColor);
}
void   TextFieldState::resetIndicatorColor() { m_indicatorColorKey.reset(); }
QColor TextFieldState::placeholderColor() const { return m_placeholderColor.value(); }
void   TextFieldState::setPlaceholderColor(const QColor& value) { m_placeholderColor = value; }
QBindable<QColor> TextFieldState::bindablePlaceholderColor() {
    return QBindable<QColor>(&m_placeholderColor);
}
void  TextFieldState::resetPlaceholderColor() { m_placeholderColorKey.reset(); }
qreal TextFieldState::placeholderOpacity() const { return m_placeholderOpacity.value(); }
void  TextFieldState::setPlaceholderOpacity(const qreal& value) { m_placeholderOpacity = value; }
QBindable<qreal> TextFieldState::bindablePlaceholderOpacity() {
    return QBindable<qreal>(&m_placeholderOpacity);
}
void TextFieldState::resetPlaceholderOpacity() { m_placeholderOpacityKey.reset(); }
token::TextFieldSizeItem TextFieldState::sizeToken() const { return m_selectedSize.value(); }
qreal TextFieldState::containerHeight() const { return sizeToken().container_height; }
qreal TextFieldState::horizontalPadding() const { return sizeToken().horizontal_padding; }
qreal TextFieldState::verticalPadding() const { return sizeToken().vertical_padding; }
qreal TextFieldState::iconSize() const { return sizeToken().icon_size; }
qreal TextFieldState::spacing() const { return sizeToken().icon_spacing; }
} // namespace qml_material
