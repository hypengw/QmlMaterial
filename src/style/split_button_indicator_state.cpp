#include "qml_material/style/split_button_indicator_state.hpp"
#include "qml_material/control/button.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
#include "qml_material/util/qml_util.hpp"
namespace qml_material
{
using Type = Enum::ButtonType;
using Size = Enum::ButtonSize;
SplitButtonIndicatorState::SplitButtonIndicatorState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    m_baseCorner.setBinding([this] {
        return calcRadius(size(), isRound(), false);
    });
    auto base = m_bindings.base();
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
    base.bind(m_appearance.stateLayerOpacity, [this]() -> qreal {
        auto* target = item();
        if (! target) return 0;
        if (target->isPressed()) return stateTokens().pressed.state_layer_opacity;
        if (target->hovered()) return stateTokens().hover.state_layer_opacity;
        if (target->visualFocus()) return stateTokens().focus.state_layer_opacity;
        return 0;
    });
    auto disabled = m_bindings.state(Interaction::Disabled);
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
    m_selection.setBinding([this] {
        return Selection { m_disabled.value() ? Interaction::Disabled : Interaction::Base,
                           targetGeneration() };
    });
    m_ready = true;
}
SplitButtonIndicatorState::~SplitButtonIndicatorState() {
    m_bindings.abandon();
    utils::disconnectAll(m_connections);
    utils::disconnectAll(m_backgroundConnections);
}
void SplitButtonIndicatorState::updateBackground() {
    utils::disconnectAll(m_backgroundConnections);
    auto* background = item() ? item()->background() : nullptr;
    if (background) {
        m_backgroundConnections.append(
            connect(background, &QQuickItem::heightChanged, this, [this, background] {
                m_backgroundHeight = background->height();
            }));
        m_backgroundConnections.append(connect(background, &QObject::destroyed, this, [this] {
            utils::disconnectAll(m_backgroundConnections);
            m_backgroundHeight = std::nullopt;
        }));
    }
    m_backgroundHeight = background ? std::optional<qreal>(background->height()) : std::nullopt;
}
Button* SplitButtonIndicatorState::item() const { return m_item.value(); }
void    SplitButtonIndicatorState::setItem(Button* value) {
    if (m_item == value) return;
    QPointer<SplitButtonIndicatorState> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        utils::disconnectAll(m_connections);
        m_item = value;
        if (value) {
            m_connections.append(connect(value,
                                         &Button::backgroundChanged,
                                         this,
                                         &SplitButtonIndicatorState::updateBackground));
            m_connections.append(connect(value, &QQuickItem::enabledChanged, this, [this] {
                m_disabled = item() && ! item()->isEnabled();
            }));
            m_connections.append(connect(value, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                QPointer<SplitButtonIndicatorState> guard(this);
                {
                    const QScopedPropertyUpdateGroup group;
                    m_item     = nullptr;
                    m_disabled = false;
                    updateBackground();
                }
                if (guard) Q_EMIT itemChanged();
            }));
        }
        updateBackground();
        setTarget(value);
        if (! guard) return;
        m_disabled = value && ! value->isEnabled();
    }
    if (guard) Q_EMIT itemChanged();
}
void SplitButtonIndicatorState::selectionChanged() {
    if (! m_ready) return;
    const auto next = m_selection.value();
    publishState(next.state == Interaction::Disabled ? QStringLiteral("disabled") : QString(),
                 [this, state = next.state] {
                     m_bindings.select(state);
                 });
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
        const auto height = m_backgroundHeight.value();
        return height ? *height / 2 : 20;
    }
    switch (Size(size)) {
    case Size::M: return 16;
    case Size::L:
    case Size::XL: return 28;
    default: return 12;
    }
}
qreal SplitButtonIndicatorState::baseCorner() const { return m_baseCorner.value(); }
#define INPUT(Type, Name, Setter, Bindable)                                              \
    Type            SplitButtonIndicatorState::Name() const { return m_##Name.value(); } \
    void            SplitButtonIndicatorState::Setter(Type value) { m_##Name = value; }  \
    QBindable<Type> SplitButtonIndicatorState::Bindable() { return QBindable<Type>(&m_##Name); }
INPUT(int, type, setType, bindableType)
INPUT(int, size, setSize, bindableSize)
INPUT(bool, isRound, setIsRound, bindableIsRound)
#undef INPUT
} // namespace qml_material
