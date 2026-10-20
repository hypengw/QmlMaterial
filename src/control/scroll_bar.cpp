#include "qml_material/control/scroll_bar.hpp"
#include <QCursor>
#include <QEvent>
#include <algorithm>
#include <cmath>

namespace qml_material
{
ScrollBar::ScrollBar(QQuickItem* parent)
    : ScrollIndicator(parent),
      m_input(this,
              [this] {
                  QPointer<ScrollBar> guard(this);
                  m_feedback.setSource(m_input.source());
                  if (guard) Q_EMIT interactionSourceChanged();
              }),
      m_feedback(this, [this] {
          QPointer<ScrollBar> guard(this);
          updateActive();
          if (guard) Q_EMIT interactionValuesChanged();
      }) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setHoverEnabled(true);
    setCursor(Qt::ArrowCursor);
    m_input.acceptsInput = [this] {
        return m_interactive;
    };
    m_input.click().keyboardEnabled = false;
    auto& drag                      = m_input.drag();
    drag.startImmediately           = true;
    drag.started                    = [this](QPointF point) {
        begin(point);
    };
    drag.moved = [this](QPointF point) {
        m_dragPoint = point;
        move(point, false);
    };
    drag.finished = [this](QPointF point, bool cancelled) {
        if (! cancelled) move(point, true);
    };
    connect(this, &Control::hoverEnabledChanged, this, [this] {
        m_input.setHoverEnabled(hoverEnabled());
    });
    connect(this, &ScrollIndicator::orientationChanged, this, &ScrollBar::cancel);
    m_feedback.setSource(m_input.source());
}
ScrollBar::~ScrollBar() = default;
ScrollBarAttached* ScrollBar::qmlAttachedProperties(QObject* object) {
    return new ScrollBarAttached(object);
}
void ScrollBar::setStepSize(qreal value) {
    if (! std::isfinite(value) || value < 0 || qFuzzyCompare(value, m_step)) return;
    m_step = value;
    Q_EMIT stepSizeChanged();
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
    const auto feedback = m_feedback.values();
    setActive(m_moving || (m_interactive && (feedback.hovered || feedback.dragged)));
}
void ScrollBar::disconnectFlickable() {
    m_moving = false;
    QPointer<ScrollBar> guard(this);
    cancel();
    if (guard) updateActive();
}
void ScrollBar::step(qreal delta) {
    if (! m_input.available()) return;
    const auto          wasActive = active();
    QPointer<ScrollBar> guard(this);
    setActive(true);
    if (! guard) return;
    requestPosition(position() + delta);
    if (guard) setActive(wasActive);
}
void ScrollBar::requestPosition(qreal value) {
    if (! std::isfinite(value) || ! m_input.available()) return;
    Q_EMIT positionChangeRequested(std::clamp(value, qreal(0), std::max(qreal(0), 1 - size())));
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
}
void ScrollBar::move(const QPointF& point, bool release) {
    if (! m_interactive || (horizontal() ? availableWidth() : availableHeight()) <= 0 ||
        minimumSize() >= 1)
        return;
    auto target =
        std::clamp(positionAt(point) - m_offset, qreal(0), std::max(qreal(0), 1 - size()));
    const auto step = m_step * (1 - size());
    if ((m_snap == SnapAlways || (release && m_snap == SnapOnRelease)) && step > 0)
        target = std::round(target / step) * step;
    requestPosition(target);
}
void ScrollBar::cancel() { m_input.cancel(); }
bool ScrollBar::event(QEvent* event) {
    QPointer<ScrollBar> guard(this);
    const bool          handled = m_input.event(event);
    if (! guard || handled) return true;
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave ||
        event->type() == QEvent::HoverMove)
        return true;
    return ScrollIndicator::event(event);
}
void ScrollBar::mouseUngrabEvent() { m_input.mouseUngrabbed(); }
void ScrollBar::touchUngrabEvent() { m_input.touchUngrabbed(); }

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
