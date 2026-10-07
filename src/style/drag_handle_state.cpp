#include "qml_material/style/drag_handle_state.hpp"
#include "qml_material/token/color.hpp"
#include "qml_material/util/qt.hpp"
namespace qml_material
{
DragHandleState::DragHandleState(QObject* parent): CommonState(parent) {
    initializeAppearance(m_bindings);
    m_radiusKey       = m_bindings.property<&DragHandleState::bindableRadius>(this);
    m_handleWidthKey  = m_bindings.property<&DragHandleState::bindableHandleWidth>(this);
    m_handleHeightKey = m_bindings.property<&DragHandleState::bindableHandleHeight>(this);
    auto base         = m_bindings.base();
    base.bind(m_appearance.textColor, colorBinding(&MdColorMgr::outline));
    base.bind(m_radiusKey, [] {
        return int(token::Shape {}.corner.full);
    });
    base.bind(m_handleWidthKey, [] {
        return 4;
    });
    base.bind(m_handleHeightKey, [] {
        return 48;
    });
    auto press = m_bindings.state(Interaction::Pressed);
    press.bind(m_appearance.textColor, colorBinding(&MdColorMgr::on_surface));
    press.bind(m_radiusKey, [] {
        return int(token::Shape {}.corner.medium);
    });
    press.bind(m_handleWidthKey, [this] {
        return handlePressedWidth();
    });
    press.bind(m_handleHeightKey, [this] {
        return handlePressedHeight();
    });
    for (auto state : { Interaction::Hovered, Interaction::Focus }) {
        auto active = m_bindings.state(state);
        active.bind(m_appearance.stateLayerColor, colorBinding(&MdColorMgr::inverse_on_surface));
        active.bind(m_appearance.stateLayerOpacity, [this, state]() -> qreal {
            return state == Interaction::Hovered ? stateTokens().hover.state_layer_opacity
                                                 : stateTokens().focus.state_layer_opacity;
        });
    }
    m_selection.setBinding([this] {
        const auto generation = targetGeneration();
        return Selection { m_disabled.value() ? Interaction::Disabled
                           : pressed()        ? Interaction::Pressed
                           : hovered()        ? Interaction::Hovered
                           : visualFocus()    ? Interaction::Focus
                                              : Interaction::Base,
                           generation };
    });
    m_ready = true;
}
DragHandleState::~DragHandleState() {
    m_bindings.abandon();
    utils::disconnectAll(m_connections);
}
QQuickItem* DragHandleState::item() const { return m_item.value(); }
void        DragHandleState::setItem(QQuickItem* value) {
    if (m_item == value) return;
    QPointer<DragHandleState> guard(this);
    {
        const QScopedPropertyUpdateGroup group;
        utils::disconnectAll(m_connections);
        updateStateInput(m_item, value);
        if (value) {
            m_connections.append(connect(value, &QQuickItem::enabledChanged, this, [this] {
                updateStateInput(m_disabled, item() && ! item()->isEnabled());
            }));
            m_connections.append(connect(value, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_connections);
                QPointer<DragHandleState> guard(this);
                {
                    const QScopedPropertyUpdateGroup group;
                    updateStateInput(m_item, nullptr);
                    updateStateInput(m_disabled, false);
                }
                if (guard && isStateActive()) Q_EMIT itemChanged();
            }));
        }
        setTarget(value);
        if (! guard) return;
        updateStateInput(m_disabled, value && ! value->isEnabled());
    }
    if (guard && isStateActive()) Q_EMIT itemChanged();
}
void DragHandleState::selectionChanged() {
    if (! m_ready) return;
    const auto selection = m_selection.value();
    QString    name;
    switch (selection.state) {
    case Interaction::Disabled: name = QStringLiteral("disabled"); break;
    case Interaction::Pressed: name = QStringLiteral("pressed"); break;
    case Interaction::Hovered: name = QStringLiteral("hovered"); break;
    case Interaction::Focus: name = QStringLiteral("focus"); break;
    default: break;
    }
    publishState(name, [this, next = selection.state] {
        m_bindings.select(next);
    });
}
bool            DragHandleState::pressed() const { return m_pressed.value(); }
void            DragHandleState::setPressed(bool value) { m_pressed = value; }
QBindable<bool> DragHandleState::bindablePressed() { return QBindable<bool>(&m_pressed); }
bool            DragHandleState::hovered() const { return m_hovered.value(); }
void            DragHandleState::setHovered(bool value) { m_hovered = value; }
QBindable<bool> DragHandleState::bindableHovered() { return QBindable<bool>(&m_hovered); }
bool            DragHandleState::visualFocus() const { return m_visualFocus.value(); }
void            DragHandleState::setVisualFocus(bool value) { m_visualFocus = value; }
QBindable<bool> DragHandleState::bindableVisualFocus() { return QBindable<bool>(&m_visualFocus); }
int             DragHandleState::handlePressedWidth() const { return m_handlePressedWidth.value(); }
void            DragHandleState::setHandlePressedWidth(int value) { m_handlePressedWidth = value; }
QBindable<int>  DragHandleState::bindableHandlePressedWidth() {
    return QBindable<int>(&m_handlePressedWidth);
}
int  DragHandleState::handlePressedHeight() const { return m_handlePressedHeight.value(); }
void DragHandleState::setHandlePressedHeight(int value) { m_handlePressedHeight = value; }
QBindable<int> DragHandleState::bindableHandlePressedHeight() {
    return QBindable<int>(&m_handlePressedHeight);
}
int            DragHandleState::radius() const { return m_radius.value(); }
void           DragHandleState::setRadius(int value) { m_radius = value; }
QBindable<int> DragHandleState::bindableRadius() { return QBindable<int>(&m_radius); }
void           DragHandleState::resetRadius() { m_radiusKey.reset(); }
int            DragHandleState::handleWidth() const { return m_handleWidth.value(); }
void           DragHandleState::setHandleWidth(int value) { m_handleWidth = value; }
QBindable<int> DragHandleState::bindableHandleWidth() { return QBindable<int>(&m_handleWidth); }
void           DragHandleState::resetHandleWidth() { m_handleWidthKey.reset(); }
int            DragHandleState::handleHeight() const { return m_handleHeight.value(); }
void           DragHandleState::setHandleHeight(int value) { m_handleHeight = value; }
QBindable<int> DragHandleState::bindableHandleHeight() { return QBindable<int>(&m_handleHeight); }
void           DragHandleState::resetHandleHeight() { m_handleHeightKey.reset(); }
} // namespace qml_material
