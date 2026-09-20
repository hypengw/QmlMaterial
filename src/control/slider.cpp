#include "qml_material/control/slider.hpp"
#include "qml_material/util/qt.hpp"
#include <QGuiApplication>
#include <QStyleHints>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QQuickWindow>
#include <algorithm>
#include <cmath>

namespace qml_material
{

Slider::Slider(QQuickItem* parent): Control(parent) {
#ifdef Q_OS_MACOS
    setFocusPolicy(Qt::TabFocus);
#else
    setFocusPolicy(Qt::StrongFocus);
#endif
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    connect(this, &Control::mirroredChanged, this, &Slider::visualPositionChanged);
    observeWindow();
}
Slider::~Slider() {
    utils::disconnectAll(m_handle_connections);
    disconnect(m_window_connection);
}

void Slider::setFrom(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_from, value)) return;
    cancel();
    m_from = value;
    Q_EMIT fromChanged();
    if (isComponentComplete()) {
        setValue(m_value);
        updatePosition();
    }
}
void Slider::setTo(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_to, value)) return;
    cancel();
    m_to = value;
    Q_EMIT toChanged();
    if (isComponentComplete()) {
        setValue(m_value);
        updatePosition();
    }
}
void Slider::setValue(qreal value) {
    if (! std::isfinite(value)) return;
    if (isComponentComplete())
        value = std::clamp(value, std::min(m_from, m_to), std::max(m_from, m_to));
    if (qFuzzyIsNull(value)) value = 0;
    if (qFuzzyCompare(m_value, value)) return;
    m_value = value;
    QPointer<Slider> guard(this);
    updatePosition();
    if (guard) Q_EMIT valueChanged();
}
void Slider::setPosition(qreal position) {
    position = std::clamp(position, qreal(0), qreal(1));
    if (qFuzzyCompare(m_position, position)) return;
    m_position = position;
    QPointer<Slider> guard(this);
    Q_EMIT positionChanged();
    if (guard) Q_EMIT visualPositionChanged();
}
void Slider::updatePosition() {
    setPosition(qFuzzyCompare(m_from, m_to) ? 0 : (m_value - m_from) / (m_to - m_from));
}
void Slider::setStepSize(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_step, value)) return;
    m_step = value;
    Q_EMIT stepSizeChanged();
}
void Slider::setSnapMode(SnapMode value) {
    if (m_snap == value) return;
    m_snap = value;
    Q_EMIT snapModeChanged();
}
void Slider::setPressed(bool value) {
    if (m_pressed == value) return;
    m_pressed = value;
    Q_EMIT pressedChanged();
}
void Slider::setLive(bool value) {
    if (m_live == value) return;
    m_live = value;
    Q_EMIT liveChanged();
}
void Slider::setWheelEnabled(bool value) {
    if (m_wheel_enabled == value) return;
    m_wheel_enabled = value;
    Q_EMIT wheelEnabledChanged();
}
void Slider::setOrientation(Qt::Orientation value) {
    if (m_orientation == value) return;
    cancel();
    m_orientation = value;
    Q_EMIT orientationChanged();
    Q_EMIT visualPositionChanged();
}
qreal Slider::implicitHandleWidth() const { return m_handle ? m_handle->implicitWidth() : 0; }
qreal Slider::implicitHandleHeight() const { return m_handle ? m_handle->implicitHeight() : 0; }
void  Slider::setHandle(QQuickItem* item) {
    if (m_handle == item) return;
    cancel();
    const auto oldWidth  = implicitHandleWidth();
    const auto oldHeight = implicitHandleHeight();
    utils::disconnectAll(m_handle_connections);
    if (m_handle) m_handle->setParentItem(nullptr);
    m_handle = item;
    if (item) {
        item->setParentItem(this);
        m_handle_connections.append(connect(
            item, &QQuickItem::implicitWidthChanged, this, &Slider::implicitHandleWidthChanged));
        m_handle_connections.append(connect(
            item, &QQuickItem::implicitHeightChanged, this, &Slider::implicitHandleHeightChanged));
        m_handle_connections.append(connect(item, &QObject::destroyed, this, [this]() {
            utils::disconnectAll(m_handle_connections);
            m_handle = nullptr;
            cancel();
            Q_EMIT handleChanged();
            Q_EMIT implicitHandleWidthChanged();
            Q_EMIT implicitHandleHeightChanged();
        }));
    }
    if (! qFuzzyCompare(oldWidth, implicitHandleWidth())) Q_EMIT implicitHandleWidthChanged();
    if (! qFuzzyCompare(oldHeight, implicitHandleHeight())) Q_EMIT implicitHandleHeightChanged();
    Q_EMIT handleChanged();
}
void Slider::setTouchDragThreshold(qreal value) {
    if (! std::isfinite(value) || qFuzzyCompare(m_touch_threshold, value)) return;
    m_touch_threshold = value;
    Q_EMIT touchDragThresholdChanged();
}
void  Slider::resetTouchDragThreshold() { setTouchDragThreshold(-1); }
qreal Slider::valueAt(qreal position) const {
    if (! std::isfinite(position)) return m_from;
    const qreal offset = (m_to - m_from) * position;
    return m_from + (qFuzzyIsNull(m_step) ? offset : std::round(offset / m_step) * m_step);
}
void  Slider::increase() { setValue(m_value + (qFuzzyIsNull(m_step) ? 0.1 : m_step)); }
void  Slider::decrease() { setValue(m_value - (qFuzzyIsNull(m_step) ? 0.1 : m_step)); }
qreal Slider::snapPosition(qreal position) const {
    if (qFuzzyCompare(m_from, m_to) || qFuzzyIsNull(m_step)) return position;
    return std::clamp((valueAt(position) - m_from) / (m_to - m_from), qreal(0), qreal(1));
}
qreal Slider::positionAt(const QPointF& point) const {
    const qreal size   = m_handle ? (horizontal() ? m_handle->width() : m_handle->height()) : 0;
    const qreal extent = (horizontal() ? availableWidth() : availableHeight()) - size;
    if (extent <= 0) return 0;
    const qreal distance = horizontal() ? (mirrored() ? width() - rightPadding() - point.x()
                                                      : point.x() - leftPadding())
                                        : height() - bottomPadding() - point.y();
    return std::clamp((distance - size / 2) / extent, qreal(0), qreal(1));
}
void Slider::moveTo(const QPointF& point, bool release) {
    const auto       sequence = m_sequence;
    QPointer<Slider> guard(this);
    const auto       old      = m_position;
    qreal            position = positionAt(point);
    if (m_snap == SnapAlways || (release && m_snap == SnapOnRelease))
        position = snapPosition(position);
    if (m_live || release) setValue(valueAt(position));
    if (! guard || m_sequence != sequence) return;
    if ((! release && (! m_live || m_snap != SnapAlways)) || (release && m_snap != NoSnap))
        setPosition(position);
    if (! guard || m_sequence != sequence) return;
    if (! qFuzzyCompare(old, position)) Q_EMIT moved();
    if (! guard || m_sequence != sequence) return;
    if (release) {
        m_input    = Input::None;
        m_touch_id = -1;
        setKeepMouseGrab(false);
        setKeepTouchGrab(false);
        setPressed(false);
    }
}
void Slider::cancel() {
    ++m_sequence;
    m_input    = Input::None;
    m_touch_id = -1;
    m_key      = 0;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    QPointer<Slider> guard(this);
    updatePosition();
    if (guard) setPressed(false);
}
void Slider::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || m_input != Input::None ||
        event->source() == Qt::MouseEventSynthesizedByQt) {
        event->ignore();
        return;
    }
    event->accept();
    m_input                   = Input::Mouse;
    const auto       sequence = ++m_sequence;
    QPointer<Slider> guard(this);
    if (focusPolicy() & Qt::ClickFocus) forceActiveFocus(Qt::MouseFocusReason);
    if (! guard || sequence != m_sequence) return;
    setKeepMouseGrab(true);
    setPressed(true);
    if (guard && sequence == m_sequence) moveTo(event->position(), false);
}
void Slider::mouseMoveEvent(QMouseEvent* event) {
    event->setAccepted(m_input == Input::Mouse);
    if (m_input == Input::Mouse) moveTo(event->position(), false);
}
void Slider::mouseReleaseEvent(QMouseEvent* event) {
    const bool accept = m_input == Input::Mouse && event->button() == Qt::LeftButton;
    event->setAccepted(accept);
    if (accept) moveTo(event->position(), true);
}
void Slider::mouseUngrabEvent() {
    if (m_input == Input::Mouse) cancel();
}
void Slider::touchUngrabEvent() {
    if (m_input == Input::Touch) cancel();
}
void Slider::touchEvent(QTouchEvent* event) {
    if (event->type() == QEvent::TouchCancel) {
        event->accept();
        if (m_input == Input::Touch) cancel();
        return;
    }
    if (m_input == Input::None) {
        for (const auto& point : event->points()) {
            if (point.state() != QEventPoint::Pressed || ! contains(point.position())) continue;
            m_input             = Input::Touch;
            const auto sequence = ++m_sequence;
            m_touch_id          = point.id();
            m_press_point       = point.position();
            event->accept();
            QPointer<Slider> guard(this);
            if (focusPolicy() & Qt::ClickFocus) forceActiveFocus(Qt::MouseFocusReason);
            if (guard && sequence == m_sequence) setPressed(true);
            return;
        }
    }
    event->setAccepted(m_input == Input::Touch);
    if (m_input != Input::Touch) return;
    for (const auto& point : event->points()) {
        if (point.id() != m_touch_id) continue;
        if (point.state() == QEventPoint::Released)
            moveTo(point.position(), true);
        else if (point.state() == QEventPoint::Updated) {
            const auto  delta     = point.position() - m_press_point;
            const qreal threshold = m_touch_threshold < 0
                                        ? QGuiApplication::styleHints()->startDragDistance()
                                        : m_touch_threshold;
            if (! keepTouchGrab() && std::abs(horizontal() ? delta.x() : delta.y()) > threshold)
                setKeepTouchGrab(true);
            if (keepTouchGrab()) moveTo(point.position(), false);
        }
        return;
    }
}
void Slider::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && m_input != Input::None) {
        event->accept();
        cancel();
        return;
    }
    const int  key    = event->key();
    const bool accept = horizontal() ? key == Qt::Key_Left || key == Qt::Key_Right
                                     : key == Qt::Key_Up || key == Qt::Key_Down;
    if (! accept || (m_input != Input::None && m_input != Input::Key)) {
        event->ignore();
        return;
    }
    event->accept();
    m_input                   = Input::Key;
    m_key                     = key;
    const auto       sequence = ++m_sequence;
    QPointer<Slider> guard(this);
    setPressed(true);
    if (! guard || sequence != m_sequence) return;
    const auto old = m_value;
    const bool up  = horizontal() ? (key == Qt::Key_Right) != mirrored() : key == Qt::Key_Up;
    if (up)
        increase();
    else
        decrease();
    if (guard && sequence == m_sequence && ! qFuzzyCompare(old, m_value)) Q_EMIT moved();
}
void Slider::keyReleaseEvent(QKeyEvent* event) {
    if (m_input != Input::Key || event->key() != m_key) {
        event->ignore();
        return;
    }
    event->accept();
    if (! event->isAutoRepeat()) cancel();
}
void Slider::wheelEvent(QWheelEvent* event) {
    if (! m_wheel_enabled) {
        event->ignore();
        return;
    }
    const auto  angle = event->angleDelta();
    const qreal delta =
        (std::abs(angle.y()) < std::abs(angle.x()) ? angle.x()
                                                   : (event->inverted() ? -angle.y() : angle.y())) /
        120.0;
    const auto       old = m_value;
    QPointer<Slider> guard(this);
    setValue(old + delta * (qFuzzyIsNull(m_step) ? 0.1 : m_step));
    event->accept();
    if (guard && ! qFuzzyCompare(old, m_value)) Q_EMIT moved();
}
void Slider::focusOutEvent(QFocusEvent* event) {
    Control::focusOutEvent(event);
    if (m_input == Input::Key) cancel();
}
void Slider::observeWindow() {
    disconnect(m_window_connection);
    if (auto win = window())
        m_window_connection = connect(win, &QWindow::activeChanged, this, [this, win]() {
            if (! win->isActive()) cancel();
        });
}
void Slider::itemChange(ItemChange change, const ItemChangeData& data) {
    Control::itemChange(change, data);
    if (change == ItemSceneChange) observeWindow();
    if (change == ItemSceneChange || change == ItemParentHasChanged ||
        (change == ItemEnabledHasChanged && ! data.boolValue) ||
        (change == ItemVisibleHasChanged && ! data.boolValue))
        cancel();
}
void Slider::componentComplete() {
    Control::componentComplete();
    setValue(m_value);
    updatePosition();
}

} // namespace qml_material
