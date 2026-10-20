#include "qml_material/control/range_slider.hpp"
#include "qml_material/input/slider_key.hpp"
#include "qml_material/util/qt.hpp"

#include <QGuiApplication>
#include <QStyleHints>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QFocusEvent>
#include <algorithm>
#include <cmath>

namespace qml_material
{
RangeSliderNode::~RangeSliderNode() { utils::disconnectAll(m_handleConnections); }
qreal RangeSliderNode::visualPosition() const {
    return m_owner->inverted() ? 1 - position() : position();
}
void RangeSliderNode::setInteractionSource(InteractionSource* source) {
    const QPointer<RangeSliderNode> guard(this);
    if (m_owner->prepareChange() && guard) m_input.setSource(source);
}
qreal RangeSliderNode::implicitHandleWidth() const {
    return m_handle ? m_handle->implicitWidth() : 0;
}
qreal RangeSliderNode::implicitHandleHeight() const {
    return m_handle ? m_handle->implicitHeight() : 0;
}
void RangeSliderNode::setHandle(QQuickItem* item) {
    if (m_handle == item) return;
    if (item && item == m_owner->node(1 - m_index)->handle()) return;
    QPointer<RangeSliderNode> guard(this);
    QPointer<QQuickItem>      target(item);
    const auto                revision = ++m_handleRevision;
    if (! m_owner->prepareChange()) return;
    if (m_handleRevision != revision) return;
    utils::disconnectAll(m_handleConnections);
    if (m_handle) m_handle->setParentItem(nullptr);
    if (! guard || m_handleRevision != revision) return;
    m_handle = target;
    if (target) {
        target->setParentItem(m_owner);
        if (! guard || m_handleRevision != revision) return;
        if (m_handle) {
            m_handleConnections << connect(
                m_handle, &QQuickItem::widthChanged, m_owner, &RangeSlider::trackGeometryChanged);
            m_handleConnections << connect(
                m_handle, &QQuickItem::heightChanged, m_owner, &RangeSlider::trackGeometryChanged);
            m_handleConnections << connect(m_handle,
                                           &QQuickItem::implicitWidthChanged,
                                           this,
                                           &RangeSliderNode::implicitHandleSizeChanged);
            m_handleConnections << connect(m_handle,
                                           &QQuickItem::implicitHeightChanged,
                                           this,
                                           &RangeSliderNode::implicitHandleSizeChanged);
            m_handleConnections << connect(m_handle, &QObject::destroyed, this, [this] {
                utils::disconnectAll(m_handleConnections);
                m_handle                           = nullptr;
                const auto                revision = ++m_handleRevision;
                QPointer<RangeSliderNode> guard(this);
                if (! m_owner->prepareChange()) return;
                if (m_handleRevision != revision) return;
                Q_EMIT handleChanged();
                if (! guard || m_handleRevision != revision) return;
                Q_EMIT implicitHandleSizeChanged();
                if (guard && m_handleRevision == revision) Q_EMIT m_owner->trackGeometryChanged();
            });
        }
    }
    Q_EMIT handleChanged();
    if (! guard || m_handleRevision != revision) return;
    Q_EMIT implicitHandleSizeChanged();
    if (guard && m_handleRevision == revision) Q_EMIT m_owner->trackGeometryChanged();
}

RangeSlider::~RangeSlider() { disconnect(m_windowConnection); }
Qt::Orientation       RangeSlider::orientation() const { return m_state.value().orientation; }
bool                  RangeSlider::live() const { return m_state.value().live; }
RangeSlider::SnapMode RangeSlider::snapMode() const { return m_state.value().snap; }
bool                  RangeSlider::wheelEnabled() const { return m_state.value().wheel; }
int                   RangeSlider::activeHandle() const { return m_state.value().active; }
int                   RangeSlider::focusedHandle() const { return m_state.value().focused; }
void                  RangeSlider::setOrientation(Qt::Orientation v) {
    if ((v != Qt::Horizontal && v != Qt::Vertical) || v == orientation() || ! prepareChange())
        return;
    auto next                   = m_state.value();
    next.orientation            = v;
    m_first.m_input.drag().axes = m_second.m_input.drag().axes = v;
    publish(next);
}
void RangeSlider::setLive(bool v) {
    if (v == live() || ! prepareChange()) return;
    auto next = m_state.value();
    next.live = v;
    publish(next);
}
void RangeSlider::setSnapMode(SnapMode v) {
    if (v < NoSnap || v > SnapOnRelease || v == snapMode() || ! prepareChange()) return;
    auto next = m_state.value();
    next.snap = v;
    publish(next);
}
void RangeSlider::setWheelEnabled(bool v) {
    auto next  = m_state.value();
    next.wheel = v;
    publish(next);
}
void RangeSlider::setFocusedHandle(int v) {
    if (v < 0 || v > 1 || v == focusedHandle() || ! prepareChange()) return;
    auto next    = m_state.value();
    next.focused = v;
    publish(next);
}
qreal RangeSlider::handleExtent(int index) const {
    auto item = index == 0 ? m_first.handle() : m_second.handle();
    return item ? (horizontal() ? item->width() : item->height()) : 0;
}
qreal RangeSlider::trackStart() const {
    const qreal available = horizontal() ? availableWidth() : availableHeight();
    return (horizontal() ? leftPadding() : topPadding()) +
           std::min(available / 2, handleExtent(inverted() ? 1 : 0) / 2);
}
qreal RangeSlider::trackLength() const {
    return std::max(qreal(0),
                    (horizontal() ? availableWidth() : availableHeight()) -
                        (handleExtent(0) + handleExtent(1)) / 2);
}
qreal RangeSlider::positionAt(const QPointF& point) const {
    if (! std::isfinite(point.x()) || ! std::isfinite(point.y())) return 0;
    const qreal length = trackLength();
    if (length <= 0) return 0;
    const qreal p = std::clamp(
        ((horizontal() ? point.x() : point.y()) - trackStart()) / length, qreal(0), qreal(1));
    return inverted() ? 1 - p : p;
}

bool RangeSlider::prepareChange() { return finish(true); }
bool RangeSlider::finish(bool cancelled) {
    if (activeHandle() < 0 && m_pointerIndex < 0 && m_keys.isEmpty()) return true;
    const int  index   = activeHandle();
    const bool started = std::exchange(m_started, false);
    m_pointerIndex     = -1;
    m_dragging         = false;
    m_keys.clear();
    const auto                  generation = ++m_generation;
    const QPointer<RangeSlider> guard(this);
    m_first.m_input.cancelActivation();
    if (! guard || generation != m_generation) return false;
    m_second.m_input.cancelActivation();
    if (! guard || generation != m_generation) return false;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    auto next          = m_state.value();
    next.active        = -1;
    const qreal length = span(next);
    for (int i = 0; i < 2; ++i)
        next.positions[i] = length > 0 ? distance(next, next.values[i]) / length : 0;
    publish(next);
    if (! guard || generation != m_generation) return false;
    if (started) Q_EMIT interactionFinished(index, cancelled);
    return guard && generation == m_generation;
}
bool RangeSlider::activate(int index) {
    if (activeHandle() == index) return true;
    auto next   = m_state.value();
    next.active = next.focused             = index;
    const auto                  generation = m_generation;
    const QPointer<RangeSlider> guard(this);
    publish(next);
    if (! guard || generation != m_generation) return false;
    m_started = true;
    Q_EMIT interactionStarted(index);
    return guard && generation == m_generation;
}
int RangeSlider::pickHandle(qreal p) const {
    if (activeHandle() >= 0) return activeHandle();
    const qreal a = nodePosition(0), b = nodePosition(1);
    const qreal da = std::abs(p - a), db = std::abs(p - b);
    if (da == db) return a > p ? 0 : 1;
    return da < db ? 0 : 1;
}
void RangeSlider::moveTo(qreal p, bool release) {
    const int index = activeHandle();
    if (index < 0) return;
    auto        next   = m_state.value();
    const qreal length = span(next);
    if (next.live || release || ! m_keys.isEmpty()) {
        next.values[index] = value(next, constrainOffset(next, index, p * length, true));
        requestChange(next);
    } else {
        const qreal offset    = constrainOffset(next, index, p * length, next.snap == SnapAlways);
        next.positions[index] = length > 0 ? offset / length : 0;
        publish(next);
    }
}
void RangeSlider::finishPointer(bool cancelled) {
    if (m_pointerIndex < 0) return;
    const QPointer<RangeSlider> guard(this);
    const auto                  generation = m_generation;
    if (! cancelled) moveTo(positionAt(m_point), true);
    if (guard && generation == m_generation) finish(cancelled);
}
void RangeSlider::pointerEvent(QEvent* event, int index) {
    if (index < 0 || index > 1 || ! m_keys.isEmpty()) {
        event->ignore();
        return;
    }
    node(index)->m_input.event(event);
}
void RangeSlider::mousePressEvent(QMouseEvent* event) {
    if (m_pointerIndex >= 0 || ! m_keys.isEmpty()) {
        event->ignore();
        return;
    }
    pointerEvent(event, pickHandle(positionAt(event->position())));
}
void RangeSlider::mouseMoveEvent(QMouseEvent* event) { pointerEvent(event, m_pointerIndex); }
void RangeSlider::mouseReleaseEvent(QMouseEvent* event) { pointerEvent(event, m_pointerIndex); }
void RangeSlider::mouseUngrabEvent() {
    if (m_pointerIndex >= 0) node(m_pointerIndex)->m_input.mouseUngrabbed();
}
void RangeSlider::touchUngrabEvent() {
    if (m_pointerIndex >= 0) node(m_pointerIndex)->m_input.touchUngrabbed();
}
void RangeSlider::touchEvent(QTouchEvent* event) {
    int index = m_pointerIndex;
    if (index < 0 && event->type() == QEvent::TouchBegin && ! event->points().isEmpty())
        index = pickHandle(positionAt(event->points().first().position()));
    pointerEvent(event, index);
}
void RangeSlider::keyPressEvent(QKeyEvent* event) {
    const int key = event->key();
    if (key == Qt::Key_Escape && activeHandle() >= 0) {
        event->accept();
        finish(true);
        return;
    }
    if (! isEnabled() || ! isVisible() || m_pointerIndex >= 0) {
        event->ignore();
        return;
    }
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            event->ignore();
            return;
        }
        const int index =
            focusedHandle() +
            ((key == Qt::Key_Backtab || event->modifiers().testFlag(Qt::ShiftModifier)) ? -1 : 1);
        event->setAccepted(index >= 0 && index <= 1);
        if (event->isAccepted()) setFocusedHandle(index);
        return;
    }
    const auto state = m_state.value();
    const auto p     = sliderKeyPosition(key,
                                         state.positions[state.focused],
                                         span(state) > 0 ? state.step / span(state) : 0,
                                         state.orientation,
                                         inverted());
    if (! p) {
        event->ignore();
        return;
    }
    event->accept();
    m_keys.insert(key);
    ++m_generation;
    if (activate(focusedHandle())) moveTo(*p, false);
}
void RangeSlider::keyReleaseEvent(QKeyEvent* event) {
    const bool accepted = m_keys.contains(event->key());
    event->setAccepted(accepted);
    if (! accepted || event->isAutoRepeat()) return;
    m_keys.remove(event->key());
    if (m_keys.isEmpty()) finish(false);
}
void RangeSlider::wheelEvent(QWheelEvent* event) {
    if (! wheelEnabled() || ! isEnabled() || ! isVisible() || activeHandle() >= 0 ||
        event->angleDelta().isNull()) {
        event->ignore();
        return;
    }
    event->accept();
    const QPoint angle = event->angleDelta();
    const qreal  delta = (std::abs(angle.y()) >= std::abs(angle.x()) ? angle.y() : angle.x()) *
                         (event->inverted() ? -1 : 1) / 120.;
    ++m_generation;
    if (! activate(focusedHandle())) return;
    const auto                  state = m_state.value();
    const QPointer<RangeSlider> guard(this);
    const auto                  generation = m_generation;
    moveTo(state.positions[state.focused] +
               (span(state) > 0 ? delta * (state.step > 0 ? state.step : .1) / span(state) : 0),
           true);
    if (guard && generation == m_generation) finish(false);
}
void RangeSlider::syncFocus() {
    const QPointer<RangeSlider> guard(this);
    m_first.m_input.setFocusEnabled(focusedHandle() == 0);
    if (guard) m_second.m_input.setFocusEnabled(focusedHandle() == 1);
}
bool RangeSlider::event(QEvent* event) {
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
        const QPointer<RangeSlider> guard(this);
        m_first.m_input.observeEvent(event);
        if (! guard) return true;
        m_second.m_input.observeEvent(event);
        if (! guard) return true;
    }
    return Control::event(event);
}
void RangeSlider::hoverAt(const QPointF& point, bool entered) {
    const QPointer<RangeSlider> guard(this);
    for (int i = 0; i < 2; ++i) {
        auto* handle = node(i)->handle();
        node(i)->m_input.observeHover(entered && handle &&
                                      handle->contains(handle->mapFromItem(this, point)));
        if (! guard) return;
    }
}
void RangeSlider::hoverEnterEvent(QHoverEvent* event) { hoverAt(event->position(), true); }
void RangeSlider::hoverMoveEvent(QHoverEvent* event) { hoverAt(event->position(), true); }
void RangeSlider::hoverLeaveEvent(QHoverEvent*) { hoverAt({}, false); }
void RangeSlider::focusOutEvent(QFocusEvent* event) {
    const QPointer<RangeSlider> guard(this);
    QQuickItem::focusOutEvent(event);
    if (guard && ! m_keys.isEmpty()) finish(true);
}
void RangeSlider::focusInEvent(QFocusEvent* event) {
    QQuickItem::focusInEvent(event);
    if (event->reason() == Qt::TabFocusReason)
        setFocusedHandle(0);
    else if (event->reason() == Qt::BacktabFocusReason)
        setFocusedHandle(1);
}
void RangeSlider::observeWindow() {
    disconnect(m_windowConnection);
    if (auto* win = window())
        m_windowConnection = connect(win, &QWindow::activeChanged, this, [this, win] {
            if (! win->isActive()) finish(true);
        });
}
void RangeSlider::itemChange(ItemChange change, const ItemChangeData& data) {
    const QPointer<RangeSlider> guard(this);
    Control::itemChange(change, data);
    if (! guard) return;
    if (change == ItemSceneChange) observeWindow();
    if (change == ItemSceneChange || change == ItemParentHasChanged ||
        (change == ItemEnabledHasChanged && ! data.boolValue) ||
        (change == ItemVisibleHasChanged && ! data.boolValue))
        finish(true);
}
} // namespace qml_material
