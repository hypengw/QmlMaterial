#include "qml_material/style/split_button_indicator_state.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qml_util.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
using Type = Enum::ButtonType;
using Size = Enum::ButtonSize;
SplitButtonIndicatorState::SplitButtonIndicatorState(QObject* parent): ButtonValueState(parent) {
    connect(this,
            &ButtonInteractionState::itemChanged,
            this,
            &SplitButtonIndicatorState::observeGeometry);
    connect(
        this, &ButtonValueState::selectedChanged, this, &SplitButtonIndicatorState::notifyGeometry);
    connect(this,
            &SplitButtonIndicatorState::sizeChanged,
            this,
            &SplitButtonIndicatorState::notifyGeometry);
    connect(this,
            &SplitButtonIndicatorState::isRoundChanged,
            this,
            &SplitButtonIndicatorState::notifyGeometry);
    connect(
        this, &CommonState::interactionsChanged, this, &SplitButtonIndicatorState::notifyGeometry);
    auto base = baseBindings();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return Type(type()) == Type::BtElevated ? elevationTokens().level1
                                                : elevationTokens().level0;
    });
    base.bind(m_appearance.corners, [this] {
        return Util::corners(innerCorner(), baseCorner(), innerCorner(), baseCorner());
    });
    base.bind(m_appearance.textColor, [this] {
        switch (Type(type())) {
        case Type::BtFilled: return color(&MdColorMgr::on_primary);
        case Type::BtFilledTonal: return color(&MdColorMgr::on_secondary_container);
        default: return color(&MdColorMgr::primary);
        }
    });
    base.bind(m_appearance.backgroundColor, [this]() -> QColor {
        switch (Type(type())) {
        case Type::BtFilled: return color(&MdColorMgr::primary);
        case Type::BtFilledTonal: return color(&MdColorMgr::secondary_container);
        case Type::BtText:
        case Type::BtOutlined: return Qt::transparent;
        default: return color(&MdColorMgr::surface_container_low);
        }
    });
    base.bind(m_appearance.stateLayerColor, [this] {
        auto* palette = colors();
        if (! palette) return QColor(Qt::transparent);
        return Type(type()) == Type::BtFilled || Type(type()) == Type::BtFilledTonal
                   ? palette->getOn(backgroundColor())
                   : palette->primary();
    });
    bindStateLayerOpacity();
    auto disabled = disabledBindings();
    disabled.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level0;
    });
    disabled.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    disabled.bind(m_appearance.backgroundColor, [this]() -> QColor {
        return Type(type()) == Type::BtFilled || Type(type()) == Type::BtFilledTonal
                   ? color(&MdColorMgr::on_surface)
                   : QColor(Qt::transparent);
    });
    disabled.bind(m_appearance.contentOpacity, [this]() -> qreal {
        return stateTokens().disabled_content;
    });
    disabled.bind(m_appearance.backgroundOpacity, [this]() -> qreal {
        return stateTokens().disabled_container;
    });
}
SplitButtonIndicatorState::~SplitButtonIndicatorState() {
    utils::disconnectAll(m_geometryConnections);
    stopBindings();
}
void SplitButtonIndicatorState::observeGeometry() {
    utils::disconnectAll(m_geometryConnections);
    if (auto* control = item()) {
        m_geometryConnections.append(connect(control,
                                             &Control::backgroundChanged,
                                             this,
                                             &SplitButtonIndicatorState::observeGeometry));
        if (auto* background = control->background())
            m_geometryConnections.append(connect(background,
                                                 &QQuickItem::heightChanged,
                                                 this,
                                                 &SplitButtonIndicatorState::notifyGeometry));
    }
    notifyGeometry();
}
void SplitButtonIndicatorState::notifyGeometry() {
    if (! isStateActive()) return;
    const QPointer<SplitButtonIndicatorState> guard(this);
    Q_EMIT baseCornerChanged();
    if (guard && isStateActive()) Q_EMIT innerCornerChanged();
}
qreal SplitButtonIndicatorState::calcRadius(int size, bool round, bool pressed) const {
    if (pressed) {
        switch (Size(size)) {
        case Size::XS:
        case Size::S: return 8;
        case Size::M: return 12;
        case Size::L:
        case Size::XL: return 16;
        }
    }
    if (round) {
        const auto height = backgroundHeight();
        return height ? *height / 2 : 20;
    }
    switch (Size(size)) {
    case Size::M: return 16;
    case Size::L:
    case Size::XL: return 28;
    default: return 12;
    }
}
qreal SplitButtonIndicatorState::baseCorner() const {
    // Geometry notifications may retarget the state; do not cache the old target's result.
    if (! pressed() && isSelected())
        return backgroundHeight().value_or(sizeToken().container_height) / 2;
    return calcRadius(size(), isRound(), false);
}
qreal SplitButtonIndicatorState::innerCorner() const {
    const auto token = sizeToken();
    if (pressed()) return token.inner_corner_pressed_size;
    return isSelected() ? baseCorner() : token.inner_corner_size;
}
token::SplitButtonSizeItem SplitButtonIndicatorState::sizeToken() const {
    const token::SplitButtonSize tokens;
    switch (Size(size())) {
    case Size::XS: return tokens.xsmall;
    case Size::M: return tokens.medium;
    case Size::L: return tokens.large;
    case Size::XL: return tokens.xlarge;
    default: return tokens.small;
    }
}
#define INPUT(Type, Name, Setter, Bindable)                                              \
    Type            SplitButtonIndicatorState::Name() const { return m_##Name.value(); } \
    void            SplitButtonIndicatorState::Setter(Type value) { m_##Name = value; }  \
    QBindable<Type> SplitButtonIndicatorState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(int, type, setType, bindableType)
INPUT(int, size, setSize, bindableSize)
INPUT(bool, isRound, setIsRound, bindableIsRound)
#undef INPUT
} // namespace qml_material
