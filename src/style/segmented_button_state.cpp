#include "qml_material/style/segmented_button_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
SegmentedButtonState::SegmentedButtonState(QObject* parent)
    : ButtonInteractionState(parent, FocusTreatment::Separate) {
    connect(this,
            &ButtonInteractionState::itemChanged,
            this,
            &SegmentedButtonState::updateMirrorSource);
    m_selectedSize.setBinding([this] {
        const auto tokens = sizeTokens();
        switch (Enum::ButtonSize(size())) {
        case Enum::ButtonSize::XS: return tokens.xsmall;
        case Enum::ButtonSize::M: return tokens.medium;
        case Enum::ButtonSize::L: return tokens.large;
        case Enum::ButtonSize::XL: return tokens.xlarge;
        default: return tokens.small;
        }
    });
    m_typescale.setBinding([this] {
        switch (Enum::ButtonSize(size())) {
        case Enum::ButtonSize::M: return token::TypeScale::default_title_medium;
        case Enum::ButtonSize::L: return token::TypeScale::default_headline_small;
        case Enum::ButtonSize::XL: return token::TypeScale::default_headline_large;
        default: return token::TypeScale::default_label_large;
        }
    });
    m_radiusKey     = bindingSet().property<&SegmentedButtonState::bindableCornerRadius>(this);
    const auto base = baseBindings();
    base.bind(m_radiusKey, [this] {
        return containerHeight() / 2;
    });
    base.bind(m_appearance.corners, [this] {
        const auto radius = cornerRadius();
        const auto pos    = Enum::ItemPosition(position());
        if (pos == Enum::ItemPosition::PosMiddle) return CornersGroup(0);
        if (pos != Enum::ItemPosition::PosFirst && pos != Enum::ItemPosition::PosLast)
            return CornersGroup(radius);
        const bool left = (pos == Enum::ItemPosition::PosFirst) != m_mirrored.value();
        return left ? CornersGroup(0, 0, radius, radius) : CornersGroup(radius, radius, 0, 0);
    });
    base.bind(m_appearance.textColor, [this] {
        return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
    });
    base.bind(m_appearance.backgroundColor, [this] {
        return checked() ? color(&MdColorMgr::secondary_container) : QColor(Qt::transparent);
    });
    base.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::outline));
    auto disabled = stateBindings(Interaction::Disabled);
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.outlineColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, [this] {
        return checked() ? color(&MdColorMgr::on_surface) : QColor(Qt::transparent);
    });
    disabled.bind(m_appearance.contentOpacity, [] {
        return qreal(.38);
    });
    disabled.bind(m_appearance.backgroundOpacity, [this] {
        return checked() ? qreal(.12) : qreal(1);
    });
    bindStateLayerOpacity();
    for (auto state : { Interaction::Pressed, Interaction::Hovered, Interaction::Focus })
        stateBindings(state).bind(m_appearance.stateLayerColor, [this] {
            return color(checked() ? &MdColorMgr::on_secondary_container : &MdColorMgr::on_surface);
        });
}
void SegmentedButtonState::updateMirrorSource() {
    disconnect(m_mirrorConnection);
    auto* control = item();
    if (control)
        m_mirrorConnection = connect(control, &Control::mirroredChanged, this, [this, control] {
            m_mirrored = control->mirrored();
        });
    m_mirrored = control && control->mirrored();
}
Button* SegmentedButtonState::item() const { return static_cast<Button*>(inputItem()); }
void    SegmentedButtonState::setItem(Button* item) { setInputItem(item); }
#define INPUT(Type, Name, Upper)                                                       \
    Type            SegmentedButtonState::Name() const { return m_##Name.value(); }    \
    void            SegmentedButtonState::set##Upper(Type value) { m_##Name = value; } \
    QBindable<Type> SegmentedButtonState::bindable##Upper() { return QBindable<Type>(&m_##Name); }
INPUT(int, position, Position)
INPUT(int, size, Size)
INPUT(qreal, cornerRadius, CornerRadius)
#undef INPUT
token::SegmentedButtonSize SegmentedButtonState::sizeTokens() const { return m_sizeTokens.value(); }
void SegmentedButtonState::setSizeTokens(const token::SegmentedButtonSize& value) {
    m_sizeTokens = value;
}
QBindable<token::SegmentedButtonSize> SegmentedButtonState::bindableSizeTokens() {
    return QBindable<token::SegmentedButtonSize>(&m_sizeTokens);
}
token::SegmentedButtonSizeItem SegmentedButtonState::sizeToken() const {
    return m_selectedSize.value();
}
token::TypeScaleItem SegmentedButtonState::typescale() const { return m_typescale.value(); }
void                 SegmentedButtonState::resetCornerRadius() { m_radiusKey.reset(); }
#define SIZE(Name, Field) \
    qreal SegmentedButtonState::Name() const { return sizeToken().Field; }
SIZE(containerHeight, container_height)
SIZE(iconSize, icon_size)
SIZE(leadingSpace, leading_space)
SIZE(trailingSpace, trailing_space)
SIZE(spacing, icon_label_space)
SIZE(outlineWidth, outline_width)
#undef SIZE
} // namespace qml_material
