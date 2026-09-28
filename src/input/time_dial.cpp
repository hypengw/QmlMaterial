#include "qml_material/input/time_dial.hpp"
#include "qml_material/token/time_picker.hpp"
#include <QGuiApplication>
#include <QStyleHints>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>
#include <cmath>
#include <numbers>

namespace qml_material
{
TimeDial::TimeDial(QQuickItem* parent): QQuickItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setActiveFocusOnTab(true);
    connect(this, &QQuickItem::enabledChanged, this, [this] {
        if (! isEnabled()) cancel();
    });
    connect(this, &QQuickItem::visibleChanged, this, [this] {
        if (! isVisible()) cancel();
    });
}
void TimeDial::setTime(TimeState* time) {
    if (time == m_time) return;
    QPointer<TimeDial>        guard(this);
    const QPointer<TimeState> requested(time);
    const auto                revision = m_revision;
    cancel();
    if (! guard || m_revision != revision + 1 || requested != time) return;
    disconnect(m_changed);
    disconnect(m_destroyed);
    m_time = time;
    if (time) {
        m_changed   = connect(time, &TimeState::changed, this, [this] {
            QPointer<TimeDial> guard(this);
            if (! m_updating) cancel();
            if (guard) Q_EMIT metricsChanged();
        });
        m_destroyed = connect(time, &QObject::destroyed, this, [this] {
            QPointer<TimeDial> guard(this);
            cancel();
            if (guard) Q_EMIT timeChanged();
            if (guard) Q_EMIT metricsChanged();
        });
    }
    Q_EMIT timeChanged();
    if (guard) Q_EMIT metricsChanged();
}
qreal TimeDial::radius() const {
    return std::max(qreal(0), std::min(width(), height()) / 2 - token::TimePicker {}.dialPadding);
}
qreal TimeDial::handAngle() const {
    if (! m_time) return 0;
    return m_time->selection() == TimeState::Hours ? (m_time->hour() % 12) * 30
                                                   : m_time->minute() * 6;
}
qreal TimeDial::handLength() const {
    return radius() * (m_time && m_time->selection() == TimeState::Hours && m_time->is24Hour() &&
                               m_time->hour() >= 12
                           ? .66
                           : 1);
}
QVariantList TimeDial::labels() const {
    QVariantList result;
    if (! m_time) return result;
    const bool minutes = m_time->selection() == TimeState::Minutes;
    const int  count   = ! minutes && m_time->is24Hour() ? 24 : 12;
    for (int i = 0; i < count; ++i) {
        const int   value    = minutes ? i * 5 : ! m_time->is24Hour() && i == 0 ? 12 : i;
        const qreal angle    = (i % 12) * std::numbers::pi / 6;
        const qreal length   = radius() * (i >= 12 ? .66 : 1);
        const int   selected = minutes ? m_time->minute()
                               : m_time->is24Hour()
                                   ? m_time->hour()
                                   : (m_time->hour() % 12 == 0 ? 12 : m_time->hour() % 12);
        result.append(QVariantMap { { "text", m_time->number(value, minutes || value == 0) },
                                    { "x", width() / 2 + std::sin(angle) * length },
                                    { "y", height() / 2 - std::cos(angle) * length },
                                    { "selected", value == selected } });
    }
    return result;
}
bool TimeDial::begin(const QPointF& point) {
    if (m_active || ! m_time || ! isEnabled() || ! m_time->acceptableInput() || radius() <= 0)
        return false;
    m_start                     = point;
    m_moved                     = false;
    m_active                    = true;
    const auto         revision = ++m_revision;
    QPointer<TimeDial> guard(this);
    forceActiveFocus(Qt::MouseFocusReason);
    if (guard && m_active && revision == m_revision) Q_EMIT pressedChanged();
    return guard && m_active && revision == m_revision;
}
void TimeDial::update(const QPointF& point, bool complete) {
    if (! m_active || ! m_time) return;
    const auto  offset   = point - QPointF(width() / 2, height() / 2);
    const qreal distance = std::hypot(offset.x(), offset.y());
    m_moved |=
        (point - m_start).manhattanLength() > QGuiApplication::styleHints()->startDragDistance();
    qreal angle = std::atan2(offset.x(), -offset.y()) * 180 / std::numbers::pi;
    if (angle < 0) angle += 360;
    const bool minutes = m_time->selection() == TimeState::Minutes;
    int        value   = minutes ? int(std::round(angle / (complete && ! m_moved ? 30 : 6))) *
                                       (complete && ! m_moved ? 5 : 1) % 60
                                 : int(std::round(angle / 30)) % 12;
    if (! minutes && m_time->is24Hour() && distance <= radius() * .66 + 12) value += 12;
    QPointer<TimeDial>        guard(this);
    const QPointer<TimeState> time = m_time;
    if (complete) {
        const auto revision = m_revision;
        cancel();
        if (! guard || m_revision != revision + 1 || time != m_time || ! time) return;
    }
    if (distance < 8) return;
    const bool format    = time->is24Hour();
    const auto selection = time->selection();
    const int  expectedValue =
        minutes ? time->hour() * 60 + value
                : (format ? value : value % 12 + time->period() * 12) * 60 + time->minute();
    m_updating = true;
    time->selectDial(value, complete);
    if (! guard) return;
    m_updating = false;
    if (m_active && (! time || time != m_time || time->value() != expectedValue ||
                     time->selection() != selection || time->is24Hour() != format))
        cancel();
}
void TimeDial::cancel() {
    ++m_revision;
    m_touchId = -1;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    if (! m_active) return;
    m_active = false;
    Q_EMIT pressedChanged();
}
void TimeDial::mousePressEvent(QMouseEvent* event) {
    if (event->source() != Qt::MouseEventNotSynthesized || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    QPointer<TimeDial> guard(this);
    if (begin(event->position()) && guard) {
        setKeepMouseGrab(true);
        event->accept();
    } else
        event->ignore();
}
void TimeDial::mouseMoveEvent(QMouseEvent* event) { update(event->position(), false); }
void TimeDial::mouseReleaseEvent(QMouseEvent* event) { update(event->position(), true); }
void TimeDial::mouseUngrabEvent() { cancel(); }
void TimeDial::touchUngrabEvent() { cancel(); }
void TimeDial::touchEvent(QTouchEvent* event) {
    if (event->type() == QEvent::TouchCancel) {
        cancel();
        event->accept();
        return;
    }
    if (event->type() == QEvent::TouchBegin) {
        if (event->points().isEmpty()) {
            event->ignore();
            return;
        }
        const auto         point = event->points().first();
        QPointer<TimeDial> guard(this);
        if (! begin(point.position()) || ! guard) {
            event->ignore();
            return;
        }
        m_touchId = point.id();
        setKeepTouchGrab(true);
    }
    for (const auto& point : event->points()) {
        if (point.id() != m_touchId) continue;
        if (point.state() != QEventPoint::State::Pressed)
            update(point.position(), point.state() == QEventPoint::State::Released);
        event->accept();
        return;
    }
    event->ignore();
}
void TimeDial::keyPressEvent(QKeyEvent* event) {
    if (! m_time || ! isEnabled()) {
        event->ignore();
        return;
    }
    event->accept();
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Right: m_time->step(1); break;
    case Qt::Key_Down:
    case Qt::Key_Left: m_time->step(-1); break;
    case Qt::Key_Home: m_time->selectDial(0); break;
    case Qt::Key_End:
        m_time->selectDial(m_time->selection() == TimeState::Minutes ? 59
                           : m_time->is24Hour()                      ? 23
                                                                     : 11);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        m_time->setSelection(m_time->selection() == TimeState::Hours ? TimeState::Minutes
                                                                     : TimeState::Hours);
        break;
    case Qt::Key_Escape: cancel(); break;
    default: event->ignore(); break;
    }
}
void TimeDial::geometryChange(const QRectF& now, const QRectF& before) {
    QQuickItem::geometryChange(now, before);
    Q_EMIT metricsChanged();
}
void TimeDial::itemChange(ItemChange change, const ItemChangeData& data) {
    QQuickItem::itemChange(change, data);
    if (change == ItemParentHasChanged || change == ItemSceneChange) {
        QPointer<TimeDial> guard(this);
        cancel();
        if (! guard) return;
        if (change == ItemSceneChange) {
            disconnect(m_windowActive);
            if (data.window)
                m_windowActive = connect(data.window, &QWindow::activeChanged, this, [this] {
                    if (! window() || ! window()->isActive()) cancel();
                });
        }
    }
}
} // namespace qml_material
