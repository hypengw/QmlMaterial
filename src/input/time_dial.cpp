#include "qml_material/input/time_dial.hpp"
#include "qml_material/token/time_picker.hpp"
#include <QKeyEvent>
#include <cmath>
#include <numbers>

namespace qml_material
{
TimeDial::TimeDial(QQuickItem* parent)
    : QQuickItem(parent), m_input(this, [this] {
          Q_EMIT interactionSourceChanged();
      }) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setActiveFocusOnTab(true);
    m_input.acceptsInput = [this] {
        return m_time && m_time->acceptableInput() && radius() > 0;
    };
    m_input.click().pointerStarted = [this](const PointerInput&) {
        forceActiveFocus(Qt::MouseFocusReason);
    };
    m_input.click().pointerClicked = [this](QPointF point) {
        update(point, true);
    };
    m_input.click().clicked = [this] {
        m_time->setSelection(m_time->selection() == TimeState::Hours ? TimeState::Minutes
                                                                     : TimeState::Hours);
    };
    auto& drag = m_input.drag();
    drag.moved = [this](QPointF point) {
        update(point, false);
    };
    drag.finished = [this](QPointF point, bool cancelled) {
        if (cancelled || ! m_time) return;
        const QPointer<TimeDial> guard(this);
        const auto               revision = m_revision;
        update(point, false);
        if (guard && revision == m_revision && m_time)
            select(m_time->selection() == TimeState::Hours
                       ? (m_time->is24Hour() ? m_time->hour() : m_time->hour() % 12)
                       : m_time->minute(),
                   true);
    };
}
TimeDial::~TimeDial() {
    disconnect(m_changed);
    disconnect(m_destroyed);
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
void TimeDial::update(const QPointF& point, bool tap) {
    if (! m_time) return;
    const auto  offset   = point - QPointF(width() / 2, height() / 2);
    const qreal distance = std::hypot(offset.x(), offset.y());
    if (distance < 8) return;
    qreal angle = std::atan2(offset.x(), -offset.y()) * 180 / std::numbers::pi;
    if (angle < 0) angle += 360;
    const bool minutes = m_time->selection() == TimeState::Minutes;
    int        value   = minutes ? int(std::round(angle / (tap ? 30 : 6))) * (tap ? 5 : 1) % 60
                                 : int(std::round(angle / 30)) % 12;
    if (! minutes && m_time->is24Hour() && distance <= radius() * .66 + 12) value += 12;
    select(value, tap);
}
void TimeDial::select(int value, bool complete) {
    if (! m_time) return;
    const QPointer<TimeState> time(m_time);
    const bool                format    = time->is24Hour();
    const auto                selection = time->selection();
    const int                 expectedValue =
        selection == TimeState::Minutes
            ? time->hour() * 60 + value
            : (format ? value : value % 12 + time->period() * 12) * 60 + time->minute();
    const auto         revision = m_revision;
    QPointer<TimeDial> guard(this);
    m_updating = true;
    time->selectDial(value, complete);
    if (! guard) return;
    m_updating = false;
    if (! complete &&
        (revision != m_revision || ! time || time != m_time || time->value() != expectedValue ||
         time->selection() != selection || time->is24Hour() != format))
        cancel();
}
void TimeDial::cancel() {
    ++m_revision;
    m_input.cancel();
}
bool TimeDial::event(QEvent* event) {
    QPointer<TimeDial> guard(this);
    const bool         handled = m_input.event(event);
    return ! guard || handled || QQuickItem::event(event);
}
void TimeDial::touchUngrabEvent() { m_input.touchUngrabbed(); }
void TimeDial::mouseUngrabEvent() { m_input.mouseUngrabbed(); }
void TimeDial::keyPressEvent(QKeyEvent* event) {
    if (! m_input.available()) {
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
    case Qt::Key_Escape: cancel(); break;
    default: event->ignore(); break;
    }
}
void TimeDial::geometryChange(const QRectF& now, const QRectF& before) {
    QPointer<TimeDial> guard(this);
    QQuickItem::geometryChange(now, before);
    if (! guard) return;
    cancel();
    if (! guard) return;
    Q_EMIT metricsChanged();
}
} // namespace qml_material
