#include "qml_material/control/abstract_button.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/util/qt.hpp"

#include <QFocusEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLineF>
#include <QMetaMethod>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTouchEvent>
#include <algorithm>

namespace qml_material
{
void AbstractButton::setIndicator(QQuickItem* value) {
    if (m_indicator == value) return;
    QPointer<AbstractButton> guard(this);
    QPointer<QQuickItem>     target(value);
    utils::disconnectAll(m_indicator_connections);
    auto old    = m_indicator;
    m_indicator = value;
    if (old && old->parentItem() == this) old->setParentItem(nullptr);
    if (! guard || m_indicator != target) return;
    if (target) {
        if (! target->parentItem()) target->setParentItem(this);
        if (! guard || ! target || m_indicator != target) return;
        m_indicator_connections.append(connect(target,
                                               &QQuickItem::implicitWidthChanged,
                                               this,
                                               &AbstractButton::implicitIndicatorWidthChanged));
        m_indicator_connections.append(connect(target,
                                               &QQuickItem::implicitHeightChanged,
                                               this,
                                               &AbstractButton::implicitIndicatorHeightChanged));
        m_indicator_connections.append(connect(target, &QObject::destroyed, this, [this] {
            m_indicator = nullptr;
            QPointer<AbstractButton> alive(this);
            Q_EMIT indicatorChanged();
            if (! alive) return;
            Q_EMIT implicitIndicatorWidthChanged();
            if (alive) Q_EMIT implicitIndicatorHeightChanged();
        }));
    }
    Q_EMIT indicatorChanged();
    if (! guard) return;
    Q_EMIT implicitIndicatorWidthChanged();
    if (guard) Q_EMIT implicitIndicatorHeightChanged();
}
AbstractButton::AbstractButton(QQuickItem* parent)
    : Control(parent),
      m_icon(new IconSpec(this)),
      m_hold_interval(QGuiApplication::styleHints()->mousePressAndHoldInterval()) {
    setFocusPolicy(Qt::StrongFocus);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    m_repeat_timer.setSingleShot(true);
    m_hold_timer.setSingleShot(true);
    m_animate_timer.setSingleShot(true);
    connect(&m_repeat_timer, &QTimer::timeout, this, &AbstractButton::repeat);
    connect(&m_hold_timer, &QTimer::timeout, this, [this]() {
        if (m_input == Input::None || ! m_pressed || ! canActivate()) return;
        m_held = true;
        Q_EMIT pressAndHold();
    });
    connect(&m_animate_timer, &QTimer::timeout, this, [this]() {
        if (m_input == Input::Simulated) finish(QPointF(width() / 2, height() / 2));
    });
    observeWindow();
}

AbstractButton::~AbstractButton() {
    utils::disconnectAll(m_indicator_connections);
    utils::disconnectAll(m_action_connections);
    if (m_group) m_group->removeButton(this);
    disconnect(m_window_connection);
    stopTimers();
}

void AbstractButton::setText(const QString& value) {
    const auto old  = text();
    m_explicit_text = true;
    m_text          = value;
    if (old != text()) Q_EMIT textChanged();
}
QString AbstractButton::text() const {
    return ! m_explicit_text && m_action ? m_action->text() : m_text;
}
void AbstractButton::resetText() {
    const auto old  = text();
    m_explicit_text = false;
    m_text.clear();
    if (old != text()) Q_EMIT textChanged();
}
Action* AbstractButton::action() const { return m_action; }
void    AbstractButton::setAction(Action* value) {
    if (m_action == value) return;
    const auto               oldText = text();
    QPointer<AbstractButton> guard(this);
    QPointer<Action>         target(value);
    const auto               sequence = ++m_sequence + (m_input != Input::None ? 1 : 0);
    cancel();
    if (! guard || m_sequence != sequence || (value && ! target)) return;
    utils::disconnectAll(m_action_connections);
    m_action         = value;
    const auto valid = [this, guard, target, sequence]() {
        return guard && m_action == target && m_sequence == sequence;
    };
    if (value) {
        m_action_connections.append(connect(value, &Action::textChanged, this, [this]() {
            if (! m_explicit_text) Q_EMIT textChanged();
        }));
        m_action_connections.append(connect(value, &Action::checkedChanged, this, [this]() {
            setChecked(m_action->isChecked());
        }));
        m_action_connections.append(connect(value, &Action::checkableChanged, this, [this]() {
            setCheckable(m_action->isCheckable());
        }));
        m_action_connections.append(connect(value, &Action::enabledChanged, this, [this]() {
            setEnabled(m_action->isEnabled());
        }));
        m_action_connections.append(connect(value, &Action::triggered, this, [this]() {
            if (isEnabled()) Q_EMIT clicked();
        }));
        m_action_connections.append(connect(value, &QObject::destroyed, this, [this]() {
            QPointer<AbstractButton> alive(this);
            utils::disconnectAll(m_action_connections);
            m_action = nullptr;
            ++m_sequence;
            cancel();
            if (! alive || m_action) return;
            m_icon->setActionIcon(nullptr);
            if (! alive) return;
            if (! m_explicit_text) Q_EMIT textChanged();
            if (alive) Q_EMIT actionChanged();
        }));
        setChecked(value->isChecked());
        if (! valid() || ! target) return;
        setCheckable(value->isCheckable());
        if (! valid() || ! target) return;
        setEnabled(value->isEnabled());
        if (! valid() || ! target) return;
    }
    m_icon->setActionIcon(m_action ? m_action->icon() : nullptr);
    if (! valid()) return;
    if (oldText != text()) Q_EMIT textChanged();
    if (valid()) Q_EMIT actionChanged();
}
ButtonGroup* AbstractButton::group() const { return m_group; }
void         AbstractButton::setGroup(ButtonGroup* value) {
    if (m_group == value) return;
    if (value)
        value->addButton(this);
    else if (m_group)
        m_group->removeButton(this);
}
void AbstractButton::setAutoExclusive(bool value) {
    if (m_auto_exclusive == value) return;
    m_auto_exclusive = value;
    QPointer<AbstractButton> guard(this);
    enforceAutoExclusive();
    if (guard) Q_EMIT autoExclusiveChanged();
}
void AbstractButton::enforceAutoExclusive() {
    if (! m_checked || ! m_auto_exclusive || m_group || ! parentItem()) return;
    QPointer<AbstractButton>        guard(this);
    QList<QPointer<AbstractButton>> siblings;
    for (auto* item : parentItem()->childItems()) {
        if (auto* button = qobject_cast<AbstractButton*>(item); button && button != this)
            siblings.append(button);
    }
    for (const auto& button : siblings) {
        if (button && button->parentItem() == parentItem() && button->autoExclusive() &&
            ! button->group())
            button->setChecked(false);
        if (! guard || ! m_checked) return;
    }
}
bool AbstractButton::canUncheck() const {
    if (m_action && ! m_action->canToggle()) return false;
    if (m_group) return ! m_group->isExclusive();
    return ! m_auto_exclusive || ! parentItem();
}
void AbstractButton::activate(bool changed) {
    if (m_action)
        m_action->triggerFromControl(this, changed);
    else
        Q_EMIT clicked();
}
void AbstractButton::setDisplay(Display value) {
    if (m_display == value) return;
    m_display = value;
    Q_EMIT displayChanged();
}
void AbstractButton::setDown(bool value) {
    const bool previous = isDown();
    m_down              = value;
    if (previous != isDown()) Q_EMIT downChanged();
}
void AbstractButton::resetDown() {
    const bool previous = isDown();
    m_down.reset();
    if (previous != isDown()) Q_EMIT downChanged();
}
void AbstractButton::setChecked(bool value) {
    if (m_checked == value) return;
    QPointer<AbstractButton> guard(this);
    m_checked = value;
    if (m_action) m_action->setChecked(value);
    if (! guard) return;
    enforceAutoExclusive();
    if (! guard) return;
    Q_EMIT checkedChanged();
}
void AbstractButton::setCheckable(bool value) {
    if (m_checkable == value) return;
    m_checkable = value;
    QPointer<AbstractButton> guard(this);
    if (m_action) m_action->setCheckable(value);
    if (! guard) return;
    Q_EMIT checkableChanged();
}
void AbstractButton::setAutoRepeat(bool value) {
    if (m_auto_repeat == value) return;
    m_auto_repeat = value;
    startTimers();
    Q_EMIT autoRepeatChanged();
}
void AbstractButton::setAutoRepeatDelay(int value) {
    value = std::max(0, value);
    if (m_repeat_delay == value) return;
    m_repeat_delay = value;
    Q_EMIT autoRepeatDelayChanged();
}
void AbstractButton::setAutoRepeatInterval(int value) {
    value = std::max(1, value);
    if (m_repeat_interval == value) return;
    m_repeat_interval = value;
    Q_EMIT autoRepeatIntervalChanged();
}
void AbstractButton::setLongPressInterval(int value) {
    value = std::max(1, value);
    if (m_hold_interval == value) return;
    m_hold_interval = value;
    Q_EMIT longPressIntervalChanged();
}

bool AbstractButton::canActivate() const {
    return isEnabled() && isVisible() && (! m_action || m_action->canTrigger());
}

void AbstractButton::changePressed(bool value) {
    if (m_pressed == value) return;
    const bool               previousDown = isDown();
    const auto               sequence     = m_sequence;
    QPointer<AbstractButton> guard(this);
    m_pressed = value;
    Q_EMIT pressedChanged();
    if (guard && sequence == m_sequence && previousDown != isDown()) Q_EMIT downChanged();
}

void AbstractButton::changePoint(const QPointF& point) {
    const auto               previous = m_point;
    const auto               sequence = m_sequence;
    QPointer<AbstractButton> guard(this);
    m_point = point;
    if (previous.x() != point.x()) Q_EMIT pressXChanged();
    if (guard && sequence == m_sequence && previous.y() != point.y()) Q_EMIT pressYChanged();
}

void AbstractButton::begin(Input input, const QPointF& point) {
    if (! canActivate() || m_input != Input::None) return;
    m_input                           = input;
    const auto               sequence = ++m_sequence;
    QPointer<AbstractButton> guard(this);
    m_origin = point;
    m_held = m_repeated = m_double_click = false;
    if (input == Input::Mouse || input == Input::Touch) pointerStarted(point);
    if (! guard || m_sequence != sequence) return;
    if (input != Input::Key && (focusPolicy() & Qt::ClickFocus))
        forceActiveFocus(Qt::MouseFocusReason);
    if (! guard || m_sequence != sequence) return;
    changePoint(point);
    if (! guard || m_sequence != sequence) return;
    changePressed(true);
    if (! guard || m_sequence != sequence) return;
    startTimers();
    Q_EMIT pressed();
}

void AbstractButton::move(const QPointF& point) {
    if (m_input == Input::None) return;
    const auto               sequence = m_sequence;
    QPointer<AbstractButton> guard(this);
    changePoint(point);
    if (! guard || m_sequence != sequence) return;
    pointerMoved(point);
    if (! guard || m_sequence != sequence) return;
    const bool wasPressed = m_pressed;
    changePressed(retainPressOutside() || contains(point));
    if (! guard || m_sequence != sequence) return;
    if (! m_pressed) {
        m_repeat_timer.stop();
        m_hold_timer.stop();
    } else if (! wasPressed && m_auto_repeat) {
        startTimers();
    }
    if (QLineF(m_origin, point).length() > QGuiApplication::styleHints()->startDragDistance())
        m_hold_timer.stop();
}

void AbstractButton::nextCheckState() { setCheckedByInteraction(! m_checked); }
void AbstractButton::setCheckedByInteraction(bool value) {
    if (! m_checkable || (m_checked && ! value && ! canUncheck())) return;
    const bool               previous = m_checked;
    QPointer<AbstractButton> guard(this);
    setChecked(value);
    if (guard && m_checked != previous) Q_EMIT toggled();
}

void AbstractButton::finish(const QPointF& point) {
    if (m_input == Input::None) return;
    const bool inside                 = m_input == Input::Key || m_input == Input::Simulated ||
                                        retainPressOutside() || contains(point);
    const bool accepted               = m_pressed && inside && canActivate();
    const bool activate               = accepted && ! m_held && ! m_repeated && ! m_double_click;
    m_input                           = Input::None;
    m_touch_id                        = -1;
    m_key                             = 0;
    const auto               sequence = ++m_sequence;
    QPointer<AbstractButton> guard(this);
    stopTimers();
    changePoint(point);
    if (! guard || m_sequence != sequence) return;
    changePressed(false);
    if (! guard || m_sequence != sequence) return;
    if (! accepted) {
        interactionEnded();
        if (! guard) return;
        Q_EMIT canceled();
        return;
    }
    const bool previousChecked = m_checked;
    if (activate) nextCheckState();
    if (! guard || m_sequence != sequence) return;
    interactionEnded();
    if (! guard || m_sequence != sequence || ! canActivate()) return;
    Q_EMIT released();
    if (guard && m_sequence == sequence && activate && canActivate())
        this->activate(previousChecked != m_checked);
}

void AbstractButton::cancel() {
    if (m_input == Input::None) return;
    m_input                           = Input::None;
    m_touch_id                        = -1;
    m_key                             = 0;
    const auto               sequence = ++m_sequence;
    QPointer<AbstractButton> guard(this);
    stopTimers();
    changePressed(false);
    if (! guard || sequence != m_sequence) return;
    interactionEnded();
    if (guard && sequence == m_sequence) Q_EMIT canceled();
}

void AbstractButton::stopTimers() {
    m_repeat_timer.stop();
    m_hold_timer.stop();
    m_animate_timer.stop();
}
void AbstractButton::startTimers() {
    m_repeat_timer.stop();
    m_hold_timer.stop();
    if (m_input == Input::None || m_input == Input::Simulated || ! m_pressed || m_held) return;
    if (m_auto_repeat)
        m_repeat_timer.start(m_repeat_delay);
    else if (m_input != Input::Key &&
             isSignalConnected(QMetaMethod::fromSignal(&AbstractButton::pressAndHold)))
        m_hold_timer.start(m_hold_interval);
}
void AbstractButton::repeat() {
    if (m_input == Input::None || ! m_pressed || ! canActivate()) return;
    const auto               sequence = m_sequence;
    QPointer<AbstractButton> guard(this);
    m_repeated                 = true;
    const bool previousChecked = m_checked;
    nextCheckState();
    if (! guard || m_sequence != sequence) return;
    Q_EMIT released();
    if (! guard || m_sequence != sequence) return;
    activate(previousChecked != m_checked);
    if (! guard || m_sequence != sequence) return;
    Q_EMIT pressed();
    if (guard && m_sequence == sequence && m_auto_repeat && m_pressed)
        m_repeat_timer.start(m_repeat_interval);
}

void AbstractButton::click() {
    if (m_input != Input::None || ! canActivate()) return;
    QPointer<AbstractButton> guard(this);
    const auto               sequence = m_sequence + 1;
    begin(Input::Simulated, QPointF(width() / 2, height() / 2));
    if (guard && m_sequence == sequence) finish(QPointF(width() / 2, height() / 2));
}
void AbstractButton::animateClick() {
    if (! canActivate() || (m_input != Input::None && m_input != Input::Simulated)) return;
    QPointer<AbstractButton> guard(this);
    const auto               sequence = m_sequence + (m_input == Input::None ? 1 : 0);
    if (m_input == Input::None) begin(Input::Simulated, QPointF(width() / 2, height() / 2));
    if (guard && m_sequence == sequence) m_animate_timer.start(100);
}
void AbstractButton::toggle() { setChecked(! m_checked); }

void AbstractButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || event->source() == Qt::MouseEventSynthesizedByQt ||
        m_input != Input::None) {
        event->ignore();
        return;
    }
    event->accept();
    begin(Input::Mouse, event->position());
}
void AbstractButton::mouseMoveEvent(QMouseEvent* event) {
    event->setAccepted(m_input == Input::Mouse);
    if (m_input == Input::Mouse) move(event->position());
}
void AbstractButton::mouseReleaseEvent(QMouseEvent* event) {
    const bool accept = m_input == Input::Mouse && event->button() == Qt::LeftButton;
    event->setAccepted(accept);
    if (accept) finish(event->position());
}
void AbstractButton::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || ! canActivate() || m_input == Input::Touch) {
        event->ignore();
        return;
    }
    event->accept();
    QPointer<AbstractButton> guard(this);
    if (m_input == Input::None) begin(Input::Mouse, event->position());
    if (! guard || m_input != Input::Mouse) return;
    if (isSignalConnected(QMetaMethod::fromSignal(&AbstractButton::doubleClicked))) {
        m_double_click = true;
        m_hold_timer.stop();
        m_repeat_timer.stop();
        Q_EMIT doubleClicked();
    }
}
void AbstractButton::mouseUngrabEvent() {
    if (m_input == Input::Mouse) cancel();
}
void AbstractButton::touchUngrabEvent() {
    if (m_input == Input::Touch) cancel();
}

void AbstractButton::touchEvent(QTouchEvent* event) {
    if (event->type() == QEvent::TouchCancel) {
        event->accept();
        if (m_input == Input::Touch) cancel();
        return;
    }
    if (m_input == Input::None) {
        for (const auto& point : event->points()) {
            if (point.state() == QEventPoint::Pressed && contains(point.position())) {
                m_touch_id = point.id();
                event->accept();
                begin(Input::Touch, point.position());
                return;
            }
        }
    }
    event->setAccepted(m_input == Input::Touch);
    if (m_input != Input::Touch) return;
    for (const auto& point : event->points()) {
        if (point.id() != m_touch_id) continue;
        if (point.state() == QEventPoint::Released)
            finish(point.position());
        else if (point.state() == QEventPoint::Updated)
            move(point.position());
        return;
    }
}
void AbstractButton::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && m_input != Input::None) {
        event->accept();
        cancel();
        return;
    }
    const bool accept = event->key() == Qt::Key_Space || event->key() == Qt::Key_Return ||
                        event->key() == Qt::Key_Enter;
    if (! accept) {
        Control::keyPressEvent(event);
        return;
    }
    event->accept();
    if (event->isAutoRepeat() || m_input != Input::None) return;
    m_key = event->key();
    begin(Input::Key, QPointF(width() / 2, height() / 2));
}
void AbstractButton::keyReleaseEvent(QKeyEvent* event) {
    if (m_input != Input::Key || event->key() != m_key) {
        Control::keyReleaseEvent(event);
        return;
    }
    event->accept();
    if (! event->isAutoRepeat()) finish(QPointF(width() / 2, height() / 2));
}
void AbstractButton::focusOutEvent(QFocusEvent* event) {
    QPointer<AbstractButton> guard(this);
    Control::focusOutEvent(event);
    if (guard && m_input != Input::Touch) cancel();
}
void AbstractButton::observeWindow() {
    disconnect(m_window_connection);
    if (auto* win = window()) {
        m_window_connection = connect(win, &QWindow::activeChanged, this, [this, win]() {
            if (! win->isActive()) cancel();
        });
    }
}
void AbstractButton::itemChange(ItemChange change, const ItemChangeData& value) {
    QPointer<AbstractButton> guard(this);
    Control::itemChange(change, value);
    if (! guard) return;
    if (change == ItemSceneChange) observeWindow();
    if (change == ItemSceneChange || change == ItemParentHasChanged ||
        (change == ItemEnabledHasChanged && ! value.boolValue) ||
        (change == ItemVisibleHasChanged && ! value.boolValue))
        cancel();
    if (guard && change == ItemParentHasChanged) enforceAutoExclusive();
}
} // namespace qml_material
