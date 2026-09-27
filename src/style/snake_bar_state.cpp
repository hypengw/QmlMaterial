#include "qml_material/style/snake_bar_state.hpp"
#include "qml_material/control/control.hpp"
#include "qml_material/control/abstract_button.hpp"
#include "qml_material/token/color.hpp"
namespace qml_material
{
SnakeBarState::SnakeBarState(QObject* parent): CommonState(parent) {
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
    base.bind(m_iconLayerColorKey, [] {
        return QColor(Qt::transparent);
    });
    base.bind(m_iconLayerOpacityKey, [] {
        return qreal(0);
    });
    for (auto interaction : { Interaction::ActionHover,
                              Interaction::ActionFocus,
                              Interaction::ActionPress,
                              Interaction::IconHover,
                              Interaction::IconFocus,
                              Interaction::IconPress }) {
        const bool action = interaction == Interaction::ActionHover ||
                            interaction == Interaction::ActionFocus ||
                            interaction == Interaction::ActionPress;
        auto       active = m_bindings.state(interaction);
        active.bind(
            action ? m_appearance.textColor : m_iconColorKey,
            colorBinding(action ? &MdColorMgr::inverse_primary : &MdColorMgr::inverse_on_surface));
        active.bind(
            action ? m_appearance.stateLayerColor : m_iconLayerColorKey,
            colorBinding(action ? &MdColorMgr::inverse_primary : &MdColorMgr::inverse_on_surface));
        active.bind(action ? m_appearance.stateLayerOpacity : m_iconLayerOpacityKey,
                    [this, interaction]() -> qreal {
                        if (interaction == Interaction::ActionHover ||
                            interaction == Interaction::IconHover)
                            return stateTokens().hover.state_layer_opacity;
                        if (interaction == Interaction::ActionFocus ||
                            interaction == Interaction::IconFocus)
                            return stateTokens().focus.state_layer_opacity;
                        return stateTokens().pressed.state_layer_opacity;
                    });
    }
    m_selection.setBinding([this] {
        auto* action = actionItem();
        auto* icon   = iconItem();
        return Selection { action && action->hovered()       ? Interaction::ActionHover
                           : action && action->visualFocus() ? Interaction::ActionFocus
                           : action && action->isDown()      ? Interaction::ActionPress
                           : icon && icon->hovered()         ? Interaction::IconHover
                           : icon && icon->visualFocus()     ? Interaction::IconFocus
                           : icon && icon->isDown()          ? Interaction::IconPress
                                                             : Interaction::Base,
                           targetGeneration() };
    });
    m_ready = true;
}
SnakeBarState::~SnakeBarState() {
    m_bindings.abandon();
    disconnect(m_actionDestroyed);
    disconnect(m_iconDestroyed);
}
Control*        SnakeBarState::item() const { return static_cast<Control*>(target()); }
void            SnakeBarState::setItem(Control* value) { setTarget(value); }
AbstractButton* SnakeBarState::actionItem() const { return m_actionItem.value(); }
void            SnakeBarState::setActionItem(AbstractButton* value) {
    if (m_actionItem == value) return;
    QPointer<SnakeBarState> guard(this);
    disconnect(m_actionDestroyed);
    if (value)
        m_actionDestroyed = connect(value, &QObject::destroyed, this, [this] {
            setActionItem(nullptr);
        });
    m_actionItem = value;
    if (guard) Q_EMIT actionItemChanged();
}
AbstractButton* SnakeBarState::iconItem() const { return m_iconItem.value(); }
void            SnakeBarState::setIconItem(AbstractButton* value) {
    if (m_iconItem == value) return;
    QPointer<SnakeBarState> guard(this);
    disconnect(m_iconDestroyed);
    if (value)
        m_iconDestroyed = connect(value, &QObject::destroyed, this, [this] {
            setIconItem(nullptr);
        });
    m_iconItem = value;
    if (guard) Q_EMIT iconItemChanged();
}
void SnakeBarState::selectionChanged() {
    if (! m_ready) return;
    const auto next = m_selection.value();
    QString    name;
    switch (next.state) {
    case Interaction::ActionHover: name = QStringLiteral("hovered:action"); break;
    case Interaction::ActionFocus: name = QStringLiteral("focus:action"); break;
    case Interaction::ActionPress: name = QStringLiteral("pressed:action"); break;
    case Interaction::IconHover: name = QStringLiteral("hovered:icon"); break;
    case Interaction::IconFocus: name = QStringLiteral("focus:icon"); break;
    case Interaction::IconPress: name = QStringLiteral("pressed:icon"); break;
    default: break;
    }
    publishState(name, [this, state = next.state] {
        m_bindings.select(state);
    });
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
