#include "qml_material/control/scroll_bar.hpp"
#include <QMouseEvent>
#include <QTouchEvent>
#include <QQuickWindow>
#include <algorithm>
#include <cmath>

namespace qml_material
{
ScrollBar::ScrollBar(QQuickItem* parent): ScrollIndicator(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setHoverEnabled(true);
    setCursor(Qt::ArrowCursor);
    connect(this, &Control::hoveredChanged, this, &ScrollBar::updateActive);
    connect(this, &ScrollIndicator::orientationChanged, this, &ScrollBar::cancel);
    observeWindow();
}
ScrollBar::~ScrollBar() { disconnect(m_window_connection); }
ScrollBarAttached* ScrollBar::qmlAttachedProperties(QObject* object) {
    return new ScrollBarAttached(object);
}
void ScrollBar::setStepSize(qreal value) {
    if (! std::isfinite(value) || value < 0 || qFuzzyCompare(value, m_step)) return;
    m_step = value;
    Q_EMIT stepSizeChanged();
}
void ScrollBar::setPressed(bool value) {
    if (m_pressed == value) return;
    m_pressed = value;
    QPointer<ScrollBar> guard(this);
    updateActive();
    if (guard) Q_EMIT pressedChanged();
}
void ScrollBar::setInteractive(bool value) {
    if (m_interactive == value) return;
    m_interactive = value;
    setAcceptedMouseButtons(value ? Qt::LeftButton : Qt::NoButton);
    setAcceptTouchEvents(value);
    if (value)
        setCursor(Qt::ArrowCursor);
    else
        unsetCursor();
    QPointer<ScrollBar> guard(this);
    if (! value) cancel();
    if (! guard) return;
    updateActive();
    if (guard) Q_EMIT interactiveChanged();
}
void ScrollBar::resetInteractive() { setInteractive(true); }
void ScrollBar::setSnapMode(SnapMode value) {
    if (value < NoSnap || value > SnapOnRelease || m_snap == value) return;
    m_snap = value;
    Q_EMIT snapModeChanged();
}
void ScrollBar::setPolicy(Policy value) {
    if (value != AsNeeded && value != AlwaysOff && value != AlwaysOn) return;
    if (m_policy == value) return;
    m_policy = value;
    Q_EMIT policyChanged();
}
void ScrollBar::setMoving(bool value) {
    m_moving = value;
    updateActive();
}
void ScrollBar::updateActive() {
    setActive(m_moving || (m_interactive && (m_pressed || hovered())));
}
void ScrollBar::disconnectFlickable() {
    m_moving = false;
    cancel();
}
void ScrollBar::step(qreal delta) {
    const auto          wasActive = active();
    QPointer<ScrollBar> guard(this);
    setActive(true);
    if (! guard) return;
    setPosition(std::clamp(position() + delta, qreal(0), 1 - size()));
    if (guard) setActive(wasActive);
}
void  ScrollBar::increase() { step(qFuzzyIsNull(m_step) ? 0.1 : m_step); }
void  ScrollBar::decrease() { step(qFuzzyIsNull(m_step) ? -0.1 : -m_step); }
qreal ScrollBar::logicalPosition(qreal visual) const {
    if (minimumSize() > size())
        return minimumSize() < 1 ? visual * (1 - size()) / (1 - minimumSize()) : 0;
    return visual;
}
qreal ScrollBar::positionAt(const QPointF& point) const {
    const auto length = horizontal() ? availableWidth() : availableHeight();
    if (length <= 0) return position();
    return logicalPosition((horizontal() ? point.x() - leftPadding() : point.y() - topPadding()) /
                           length);
}
void ScrollBar::begin(const QPointF& point) {
    m_offset          = positionAt(point) - position();
    const auto extent = std::max(size(), logicalPosition(minimumSize()));
    if (m_offset < 0 || m_offset > extent) m_offset = extent / 2;
    setPressed(true);
}
void ScrollBar::move(const QPointF& point, bool release) {
    if (! m_pressed || ! m_interactive ||
        (horizontal() ? availableWidth() : availableHeight()) <= 0 || minimumSize() >= 1)
        return;
    auto       target = std::clamp(positionAt(point) - m_offset, qreal(0), 1 - size());
    const auto step   = m_step * (1 - size());
    if ((m_snap == SnapAlways || (release && m_snap == SnapOnRelease)) && step > 0)
        target = std::round(target / step) * step;
    setPosition(std::clamp(target, qreal(0), 1 - size()));
}
void ScrollBar::cancel() {
    QPointer<ScrollBar> guard(this);
    const bool          mouse = m_mouse;
    const bool          touch = m_touch_id >= 0;
    m_mouse                   = false;
    m_touch_id                = -1;
    m_offset                  = 0;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    if (mouse) ungrabMouse();
    if (! guard) return;
    if (touch) ungrabTouchPoints();
    if (! guard) return;
    setPressed(false);
    if (guard) updateActive();
}
void ScrollBar::mousePressEvent(QMouseEvent* event) {
    if (! m_interactive || event->button() != Qt::LeftButton || m_mouse || m_touch_id >= 0) {
        event->ignore();
        return;
    }
    event->accept();
    m_mouse = true;
    setKeepMouseGrab(true);
    QPointer<ScrollBar> guard(this);
    begin(event->position());
    if (guard && m_mouse) move(event->position(), false);
}
void ScrollBar::mouseMoveEvent(QMouseEvent* event) {
    if (! m_mouse) {
        event->ignore();
        return;
    }
    event->accept();
    move(event->position(), false);
}
void ScrollBar::mouseReleaseEvent(QMouseEvent* event) {
    if (! m_mouse || event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    event->accept();
    QPointer<ScrollBar> guard(this);
    move(event->position(), true);
    if (guard) cancel();
}
void ScrollBar::mouseUngrabEvent() { cancel(); }
void ScrollBar::touchUngrabEvent() { cancel(); }
void ScrollBar::touchEvent(QTouchEvent* event) {
    if (event->type() == QEvent::TouchCancel) {
        cancel();
        event->accept();
        return;
    }
    if (! m_interactive || m_mouse) {
        event->ignore();
        return;
    }
    for (const auto& point : event->points()) {
        if (m_touch_id < 0 && point.state() == QEventPoint::Pressed) {
            m_touch_id = point.id();
            setKeepTouchGrab(true);
            event->accept();
            begin(point.position());
            return;
        }
        if (point.id() != m_touch_id) continue;
        event->accept();
        QPointer<ScrollBar> guard(this);
        if (point.state() == QEventPoint::Released) {
            move(point.position(), true);
            if (guard) cancel();
        } else if (point.state() == QEventPoint::Updated)
            move(point.position(), false);
        return;
    }
    event->ignore();
}
void ScrollBar::observeWindow() {
    disconnect(m_window_connection);
    if (auto win = window())
        m_window_connection = connect(win, &QWindow::activeChanged, this, [this, win]() {
            if (! win->isActive()) cancel();
        });
}
void ScrollBar::itemChange(ItemChange change, const ItemChangeData& data) {
    ScrollIndicator::itemChange(change, data);
    if (change == ItemSceneChange) observeWindow();
    if (change == ItemSceneChange || change == ItemParentHasChanged ||
        (change == ItemEnabledHasChanged && ! data.boolValue) ||
        (change == ItemVisibleHasChanged && ! data.boolValue))
        cancel();
}

ScrollBarAttached::ScrollBarAttached(QObject* parent)
    : QObject(parent), m_attachment(new ScrollIndicatorAttached(parent, this, true)) {
    connect(m_attachment,
            &ScrollIndicatorAttached::horizontalChanged,
            this,
            &ScrollBarAttached::horizontalChanged);
    connect(m_attachment,
            &ScrollIndicatorAttached::verticalChanged,
            this,
            &ScrollBarAttached::verticalChanged);
}
ScrollBar* ScrollBarAttached::horizontal() const {
    return qobject_cast<ScrollBar*>(m_attachment->horizontal());
}
ScrollBar* ScrollBarAttached::vertical() const {
    return qobject_cast<ScrollBar*>(m_attachment->vertical());
}
void ScrollBarAttached::setHorizontal(ScrollBar* item) { m_attachment->setHorizontal(item); }
void ScrollBarAttached::setVertical(ScrollBar* item) { m_attachment->setVertical(item); }
} // namespace qml_material
