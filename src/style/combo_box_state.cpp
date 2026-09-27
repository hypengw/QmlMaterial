#include "qml_material/style/combo_box_state.hpp"
#include "qml_material/control/combo_box.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
ComboBoxState::ComboBoxState(QObject* parent): InputState(parent) {
    m_hovered.setBinding([this] {
        return item() && item()->hovered();
    });
    m_focused.setBinding([this] {
        return item() && item()->visualFocus();
    });
    m_acceptable.setBinding([this] {
        return ! item() || item()->acceptableInput();
    });
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
    m_typescaleKey    = m_bindings.property<&ComboBoxState::bindableTypescale>(this);
    m_labelColorKey   = m_bindings.property<&ComboBoxState::bindableLabelColor>(this);
    m_labelOpacityKey = m_bindings.property<&ComboBoxState::bindableLabelOpacity>(this);
    bindInputAppearance(m_labelColorKey, m_labelOpacityKey);
    auto base = m_bindings.base();
    base.bind(m_typescaleKey, [this] {
        return sizeToken().type_scale;
    });
    auto disabled = m_bindings.state(Interaction::Disabled);
    disabled.bind(m_appearance.contentOpacity, [] {
        return qreal(.38);
    });
    disabled.bind(m_appearance.backgroundOpacity, [] {
        return qreal(.12);
    });
}
ComboBoxState::~ComboBoxState() {
    m_bindings.abandon();
    m_hovered.takeBinding();
    m_focused.takeBinding();
    m_acceptable.takeBinding();
}
ComboBox*           ComboBoxState::item() const { return static_cast<ComboBox*>(inputItem()); }
void                ComboBoxState::setItem(ComboBox* item) { setInputItem(item); }
int                 ComboBoxState::size() const { return m_size.value(); }
void                ComboBoxState::setSize(const int& value) { m_size = value; }
QBindable<int>      ComboBoxState::bindableSize() { return QBindable<int>(&m_size); }
token::ComboBoxSize ComboBoxState::sizeTokens() const { return m_sizeTokens.value(); }
void ComboBoxState::setSizeTokens(const token::ComboBoxSize& value) { m_sizeTokens = value; }
QBindable<token::ComboBoxSize> ComboBoxState::bindableSizeTokens() {
    return QBindable<token::ComboBoxSize>(&m_sizeTokens);
}
token::TypeScaleItem ComboBoxState::typescale() const { return m_typescale.value(); }
void ComboBoxState::setTypescale(const token::TypeScaleItem& value) { m_typescale = value; }
QBindable<token::TypeScaleItem> ComboBoxState::bindableTypescale() {
    return QBindable<token::TypeScaleItem>(&m_typescale);
}
void              ComboBoxState::resetTypescale() { m_typescaleKey.reset(); }
QColor            ComboBoxState::labelColor() const { return m_labelColor.value(); }
void              ComboBoxState::setLabelColor(const QColor& value) { m_labelColor = value; }
QBindable<QColor> ComboBoxState::bindableLabelColor() { return QBindable<QColor>(&m_labelColor); }
void              ComboBoxState::resetLabelColor() { m_labelColorKey.reset(); }
qreal             ComboBoxState::labelOpacity() const { return m_labelOpacity.value(); }
void              ComboBoxState::setLabelOpacity(const qreal& value) { m_labelOpacity = value; }
QBindable<qreal> ComboBoxState::bindableLabelOpacity() { return QBindable<qreal>(&m_labelOpacity); }
void             ComboBoxState::resetLabelOpacity() { m_labelOpacityKey.reset(); }
token::ComboBoxSizeItem ComboBoxState::sizeToken() const { return m_selectedSize.value(); }
qreal ComboBoxState::containerHeight() const { return sizeToken().container_height; }
qreal ComboBoxState::horizontalPadding() const { return sizeToken().horizontal_padding; }
qreal ComboBoxState::indicatorSize() const { return sizeToken().indicator_size; }
qreal ComboBoxState::spacing() const { return sizeToken().indicator_spacing; }
} // namespace qml_material
