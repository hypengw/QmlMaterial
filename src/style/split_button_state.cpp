#include "qml_material/style/split_button_state.hpp"
#include "qml_material/util/qml_util.hpp"
namespace qml_material
{
SplitButtonState::SplitButtonState(QObject* parent): ButtonAppearanceState(parent) {
    connect(this, &SplitButtonState::sizeTokenChanged, this, &SplitButtonState::geometryChanged);
    connect(this, &SplitButtonState::sizeChanged, this, &SplitButtonState::geometryChanged);
    m_sizeToken.setBinding([this] {
        const auto tokens = sizeTokens();
        switch (Enum::ButtonSize(size())) {
        case Enum::ButtonSize::XS: return tokens.xsmall;
        case Enum::ButtonSize::S: return tokens.small;
        case Enum::ButtonSize::L: return tokens.large;
        case Enum::ButtonSize::XL: return tokens.xlarge;
        default: return tokens.medium;
        }
    });
    m_innerCorner.setBinding([this] {
        const auto token = sizeToken();
        return down()      ? token.inner_corner_pressed_size
               : hovered() ? token.inner_corner_hovered_size
                           : token.inner_corner_size;
    });
    m_trailingCornersKey = bindingSet().property<&SplitButtonState::bindableTrailingCorners>(this);
    auto base            = baseBindings();
    base.bind(m_appearance.corners, [this] {
        return Util::corners(outerCorner(), innerCorner(), outerCorner(), innerCorner());
    });
    base.bind(m_trailingCornersKey, [this] {
        return Util::corners(innerCorner(), outerCorner(), innerCorner(), outerCorner());
    });
    base.bind(m_appearance.corner, [this]() -> qreal {
        const auto token = buttonSize();
        if (down()) return token.pressed_corner_size;
        if (! isRound()) return token.corner_size;
        return backgroundHeight().value_or(token.container_height) / 2;
    });
}
token::SplitButtonSize SplitButtonState::sizeTokens() const { return m_sizeTokens.value(); }
void SplitButtonState::setSizeTokens(const token::SplitButtonSize& value) { m_sizeTokens = value; }
QBindable<token::SplitButtonSize> SplitButtonState::bindableSizeTokens() {
    return QBindable<token::SplitButtonSize>(&m_sizeTokens);
}
token::SplitButtonSizeItem SplitButtonState::sizeToken() const { return m_sizeToken.value(); }
token::ButtonSizeItem      SplitButtonState::buttonSize() const {
    const token::ButtonSize tokens;
    switch (Enum::ButtonSize(size())) {
    case Enum::ButtonSize::XS: return tokens.xsmall;
    case Enum::ButtonSize::M: return tokens.medium;
    case Enum::ButtonSize::L: return tokens.large;
    case Enum::ButtonSize::XL: return tokens.xlarge;
    default: return tokens.small;
    }
}
qreal SplitButtonState::containerHeight() const { return sizeToken().container_height; }
qreal SplitButtonState::iconSize() const { return buttonSize().icon_size; }
qreal SplitButtonState::spacing() const { return buttonSize().spacing; }
qreal SplitButtonState::leadingSpace() const { return sizeToken().leading_button_leading_space; }
qreal SplitButtonState::trailingSpace() const { return sizeToken().leading_button_trailing_space; }
qreal SplitButtonState::innerCorner() const { return m_innerCorner.value(); }
qreal SplitButtonState::outerCorner() const { return sizeToken().outer_corner_size; }
qreal SplitButtonState::betweenSpace() const { return sizeToken().between_space; }
CornersGroup SplitButtonState::trailingCorners() const { return m_trailingCorners.value(); }
void SplitButtonState::setTrailingCorners(const CornersGroup& value) { m_trailingCorners = value; }
void SplitButtonState::resetTrailingCorners() { m_trailingCornersKey.reset(); }
QBindable<CornersGroup> SplitButtonState::bindableTrailingCorners() {
    return QBindable<CornersGroup>(&m_trailingCorners);
}
} // namespace qml_material
