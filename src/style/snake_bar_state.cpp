#include "qml_material/style/snake_bar_state.hpp"
#include "qml_material/control/control.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
SnakeBarState::SnakeBarState(QObject* parent)
    : CommonState(parent),
      m_actionIndication(this,
                         [this] {
                             if (isStateActive()) Q_EMIT actionIndicationChanged();
                         }),
      m_iconIndication(this, [this] {
          if (isStateActive()) Q_EMIT iconIndicationChanged();
      }) {
    m_actionIndication.bindDefaultColor(Qt::makePropertyBinding([this] {
        return textColor();
    }));
    m_iconIndication.bindDefaultColor(Qt::makePropertyBinding([this] {
        return iconColor();
    }));
    for (auto* indication : { &m_actionIndication, &m_iconIndication })
        indication->bindDefaultTokens(Qt::makePropertyBinding([this] {
            return stateTokens();
        }));
    initializeAppearance(m_bindings);
    connect(this, &CommonState::targetChanged, this, &SnakeBarState::itemChanged);
    m_iconColorKey      = m_bindings.property<&SnakeBarState::bindableIconColor>(this);
    m_iconLayerColorKey = m_bindings.property<&SnakeBarState::bindableIconStateLayerColor>(this);
    m_iconLayerOpacityKey =
        m_bindings.property<&SnakeBarState::bindableIconStateLayerOpacity>(this);
    auto base = m_bindings.base();
    base.bind(m_appearance.elevation, [this]() -> qreal {
        return elevationTokens().level3;
    });
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::inverse_primary));
    base.bind(m_appearance.supportTextColor, colorBinding(&MdColorMgr::inverse_on_surface));
    base.bind(m_appearance.backgroundColor, colorBinding(&MdColorMgr::inverse_surface));
    base.bind(m_iconColorKey, colorBinding(&MdColorMgr::inverse_on_surface));
    base.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::inverse_primary));
    base.bind(m_iconLayerColorKey, colorBinding(&MdColorMgr::inverse_on_surface));
    const auto opacity = [this](const IndicationPolicy& policy) -> qreal {
        const auto values = policy.interactions().values();
        if (values.pressed) return stateTokens().pressed.state_layer_opacity;
        return policy.values().opacity;
    };
    base.bind(m_appearance.stateLayerOpacity, [this, opacity] {
        return opacity(m_actionIndication);
    });
    base.bind(m_iconLayerOpacityKey, [this, opacity] {
        return opacity(m_iconIndication);
    });
}
SnakeBarState::~SnakeBarState() {
    m_bindings.abandon();
    disconnect(m_actionDestroyed);
    disconnect(m_iconDestroyed);
    disconnect(m_actionSourceChanged);
    disconnect(m_iconSourceChanged);
}
Control*       SnakeBarState::item() const { return static_cast<Control*>(target()); }
void           SnakeBarState::setItem(Control* value) { setTarget(value); }
ButtonControl* SnakeBarState::actionItem() const { return m_actionItem.value(); }
void           SnakeBarState::setActionItem(ButtonControl* value) {
    if (m_actionItem == value) return;
    QPointer<SnakeBarState> guard(this);
    QPointer<ButtonControl> input(value);
    disconnect(m_actionDestroyed);
    disconnect(m_actionSourceChanged);
    if (value)
        m_actionSourceChanged =
            connect(value, &ButtonControl::interactionSourceChanged, this, [this] {
                auto* item = actionItem();
                m_actionIndication.setSource(item ? item->interactionSource() : nullptr);
            });
    if (value)
        m_actionDestroyed = connect(value, &QObject::destroyed, this, [this] {
            setActionItem(nullptr);
        });
    updateStateInput(m_actionItem, value);
    if (guard && isStateActive() && actionItem() == input) {
        m_actionIndication.setTarget(input.data());
        if (guard && actionItem() == input)
            m_actionIndication.setSource(input ? input->interactionSource() : nullptr);
    }
    if (guard && isStateActive()) Q_EMIT actionItemChanged();
}
ButtonControl* SnakeBarState::iconItem() const { return m_iconItem.value(); }
void           SnakeBarState::setIconItem(ButtonControl* value) {
    if (m_iconItem == value) return;
    QPointer<SnakeBarState> guard(this);
    QPointer<ButtonControl> input(value);
    disconnect(m_iconDestroyed);
    disconnect(m_iconSourceChanged);
    if (value)
        m_iconSourceChanged =
            connect(value, &ButtonControl::interactionSourceChanged, this, [this] {
                auto* item = iconItem();
                m_iconIndication.setSource(item ? item->interactionSource() : nullptr);
            });
    if (value)
        m_iconDestroyed = connect(value, &QObject::destroyed, this, [this] {
            setIconItem(nullptr);
        });
    updateStateInput(m_iconItem, value);
    if (guard && isStateActive() && iconItem() == input) {
        m_iconIndication.setTarget(input.data());
        if (guard && iconItem() == input)
            m_iconIndication.setSource(input ? input->interactionSource() : nullptr);
    }
    if (guard && isStateActive()) Q_EMIT iconItemChanged();
}
QColor            SnakeBarState::iconColor() const { return m_iconColor.value(); }
void              SnakeBarState::setIconColor(const QColor& value) { m_iconColor = value; }
QBindable<QColor> SnakeBarState::bindableIconColor() { return QBindable<QColor>(&m_iconColor); }
void              SnakeBarState::resetIconColor() { m_iconColorKey.reset(); }
QColor SnakeBarState::iconStateLayerColor() const { return m_iconStateLayerColor.value(); }
void   SnakeBarState::setIconStateLayerColor(const QColor& value) { m_iconStateLayerColor = value; }
QBindable<QColor> SnakeBarState::bindableIconStateLayerColor() {
    return QBindable<QColor>(&m_iconStateLayerColor);
}
void  SnakeBarState::resetIconStateLayerColor() { m_iconLayerColorKey.reset(); }
qreal SnakeBarState::iconStateLayerOpacity() const { return m_iconStateLayerOpacity.value(); }
void  SnakeBarState::setIconStateLayerOpacity(qreal value) { m_iconStateLayerOpacity = value; }
QBindable<qreal> SnakeBarState::bindableIconStateLayerOpacity() {
    return QBindable<qreal>(&m_iconStateLayerOpacity);
}
void SnakeBarState::resetIconStateLayerOpacity() { m_iconLayerOpacityKey.reset(); }
} // namespace qml_material
