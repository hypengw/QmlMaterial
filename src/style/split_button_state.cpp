#include "qml_material/style/split_button_state.hpp"
#include "qml_material/control/button_control.hpp"
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
        return pressed() ? token.inner_corner_pressed_size : token.inner_corner_size;
    });
    m_trailingCornersKey = bindingSet().property<&SplitButtonState::bindableTrailingCorners>(this);
    auto base            = baseBindings();
    bindCornerShape(
        [this]() -> CornerShape {
            const auto full     = fullCorner(containerHeight());
            auto       absolute = Util::corners(0, innerCorner(), 0, innerCorner());
            auto       relative = Util::corners(1, 0, 1, 0);
            if (! isRound()) {
                absolute = Util::corners(buttonSize().corner_size,
                                         innerCorner(),
                                         buttonSize().corner_size,
                                         innerCorner());
                relative = CornersGroup(0);
            }
            if (inputItem() && inputItem()->mirrored()) {
                std::swap(absolute[0], absolute[1]);
                std::swap(absolute[2], absolute[3]);
                std::swap(relative[0], relative[1]);
                std::swap(relative[2], relative[3]);
            }
            return { absolute, relative, full };
        },
        false);
    base.bind(m_trailingCornersKey, [this] {
        return Util::corners(innerCorner(), outerCorner(), innerCorner(), outerCorner());
    });
    base.bind(m_appearance.corner, [this]() -> qreal {
        const auto token = buttonSize();
        if (pressed()) return token.pressed_corner_size;
        if (! isRound()) return token.corner_size;
        return fullCorner(token.container_height);
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
