#include "scroll_participant_p.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include <QtQuick/private/qquickflickable_p_p.h>
#include <algorithm>

namespace qml_material
{
namespace
{
template<typename State>
void releaseNativeGrab(State* state) {
    // Qt renamed this field after 6.8.
    if constexpr (requires { state->stealGrab; })
        state->stealGrab = false;
    else
        state->stealMouse = false;
    state->pressed = false;
}
struct QtViewportAccess : QQuickFlickable {
    static QPointF bound(QQuickFlickable* item, QPointF p) {
        const auto minX = -((item->*&QtViewportAccess::minXExtent)());
        const auto minY = -((item->*&QtViewportAccess::minYExtent)());
        const auto maxX = std::max(minX, -((item->*&QtViewportAccess::maxXExtent)()));
        const auto maxY = std::max(minY, -((item->*&QtViewportAccess::maxYExtent)()));
        return { std::clamp(p.x(), minX, maxX), std::clamp(p.y(), minY, maxY) };
    }
    static Qt::Orientations directions(QQuickFlickable* item) {
        Qt::Orientations axes;
        if ((item->*&QtViewportAccess::xflick)()) axes |= Qt::Horizontal;
        if ((item->*&QtViewportAccess::yflick)()) axes |= Qt::Vertical;
        return axes;
    }
    static void moving(QQuickFlickable* item, bool start) {
        if (start)
            (item->*&QtViewportAccess::movementStarting)();
        else
            (item->*static_cast<void (QQuickFlickable::*)()>(&QtViewportAccess::movementEnding))();
    }
};
} // namespace
ScrollParticipant::ScrollParticipant(QQuickItem* item)
    : m_owned(qobject_cast<Flickable*>(item)), m_qt(qobject_cast<QQuickFlickable*>(item)) {}
QQuickItem* ScrollParticipant::item() const {
    return m_owned ? static_cast<QQuickItem*>(m_owned.data()) : m_qt.data();
}
bool ScrollParticipant::available() const {
    auto* view = item();
    return view && view->isVisible() && view->isEnabled() &&
           (m_owned ? m_owned->isInteractive() : m_qt->isInteractive());
}
int ScrollParticipant::pressDelay() const {
    if (m_qt && m_qt->pressDelay() > 0) return 0;
    auto* config =
        qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(item(), false));
    return config ? config->pressDelay() : 100;
}
QPointF ScrollParticipant::position() const {
    if (m_owned) return { m_owned->contentX(), m_owned->contentY() };
    if (m_qt) return { m_qt->contentX(), m_qt->contentY() };
    return {};
}
QPointF ScrollParticipant::bounded(QPointF p) const {
    if (m_owned) return m_owned->boundedScrollPosition(p);
    return m_qt ? QtViewportAccess::bound(m_qt, p) : QPointF();
}
Qt::Orientations ScrollParticipant::axes() const {
    if (m_qt) return QtViewportAccess::directions(m_qt);
    return m_owned ? m_owned->scrollAxes() : Qt::Orientations();
}
bool ScrollParticipant::accepts(QPointF scene) const {
    if (! available()) return false;
    const auto p = item()->mapFromScene(scene);
    return m_owned ? m_owned->acceptsScrollPoint(p) : m_qt->contains(p);
}
qreal ScrollParticipant::deceleration() const {
    return m_owned ? m_owned->flickDeceleration() : m_qt ? m_qt->flickDeceleration() : 1500;
}
qreal ScrollParticipant::maximumVelocity() const {
    return m_owned ? m_owned->maximumFlickVelocity() : m_qt ? m_qt->maximumFlickVelocity() : 2500;
}
bool ScrollParticipant::synchronousDrag() const {
    return m_owned ? m_owned->synchronousDrag() : m_qt && m_qt->synchronousDrag();
}
void ScrollParticipant::begin(Flickable::ScrollSource source) {
    if (m_owned) {
        m_owned->beginScroll(source);
    } else if (m_qt) {
        auto* d = QQuickFlickablePrivate::get(m_qt);
        d->clearDelayedPress();
        d->cancelInteraction();
        if (! m_qt) return;
        releaseNativeGrab(d);
        if (m_qt) m_qt->cancelFlick();
    }
}
QPointF ScrollParticipant::consume(QPointF delta, Activity activity, QPointF velocity,
                                   const std::function<bool()>& current) {
    const auto live = [&] {
        return available() && current();
    };
    if (! live()) return {};
    if (m_owned) {
        const auto source = activity == Activity::Drag    ? Flickable::ScrollSource::Drag
                            : activity == Activity::Fling ? Flickable::ScrollSource::Fling
                                                          : Flickable::ScrollSource::Wheel;
        return m_owned->applyScrollDelta(delta, source, velocity).consumed;
    }
    const auto before    = position();
    auto       after     = bounded(before + delta);
    const auto direction = axes();
    if (! direction.testFlag(Qt::Horizontal)) after.setX(before.x());
    if (! direction.testFlag(Qt::Vertical)) after.setY(before.y());
    if (after == before) return {};
    const bool x = after.x() != before.x(), y = after.y() != before.y();
    if (m_qt) {
        auto* d = QQuickFlickablePrivate::get(m_qt);
        d->hData.move.setValue(-after.x());
        if (! live()) return after - before;
        d->vData.move.setValue(-after.y());
        if (! live()) return after - before;
        d->hMoved |= x;
        d->vMoved |= y;
        if (activity == Activity::Drag) d->draggingStarting();
        if (! live()) return after - before;
        if (activity == Activity::Fling) d->flickingStarted(x, y);
        if (! live()) return after - before;
        QtViewportAccess::moving(m_qt, true);
        if (! live()) return after - before;
        d->hData.smoothVelocity.setValue(x ? velocity.x() : 0);
        if (! live()) return after - before;
        d->vData.smoothVelocity.setValue(y ? velocity.y() : 0);
    }
    return position() - before;
}
void ScrollParticipant::release(QPointF velocity) {
    if (m_owned)
        m_owned->releaseScroll(velocity);
    else if (m_qt) {
        auto* d = QQuickFlickablePrivate::get(m_qt);
        releaseNativeGrab(d);
        d->draggingEnding();
    }
}
void ScrollParticipant::end(bool cancelled) {
    if (m_owned) {
        m_owned->endScroll(cancelled ? Flickable::ScrollEnd::Cancelled
                                     : Flickable::ScrollEnd::Finished);
    } else if (m_qt) {
        auto* d = QQuickFlickablePrivate::get(m_qt);
        releaseNativeGrab(d);
        d->draggingEnding();
        if (m_qt) m_qt->cancelFlick();
        if (m_qt) QtViewportAccess::moving(m_qt, false);
    }
}
} // namespace qml_material
