#include "qml_material/control/range_slider.hpp"
#include "qml_material/util/qt.hpp"

#include <QGuiApplication>
#include <QStyleHints>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace qml_material
{
RangeSliderNode::~RangeSliderNode() { utils::disconnectAll(m_handleConnections); }
qreal RangeSliderNode::visualPosition() const {
    return m_owner->inverted() ? 1 - position() : position();
}
bool RangeSliderNode::pressed() const { return m_owner->activeHandle() == m_index; }
bool RangeSliderNode::focused() const {
    return m_owner->visualFocus() && m_owner->focusedHandle() == m_index;
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
    auto next        = m_state.value();
    next.orientation = v;
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
    if (m_input == Input::None) return true;
    const int  index    = activeHandle();
    const bool started  = m_started;
    m_started           = false;
    m_input             = Input::None;
    m_touchId           = -1;
    m_key               = 0;
    const auto sequence = ++m_sequence;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    auto next          = m_state.value();
    next.active        = -1;
    const qreal length = span(next);
    for (int i = 0; i < 2; ++i)
        next.positions[i] = length > 0 ? distance(next, next.values[i]) / length : 0;
    QPointer<RangeSlider> guard(this);
    publish(next);
    if (guard && started) Q_EMIT interactionFinished(index, cancelled);
    return guard && m_sequence == sequence;
}
bool RangeSlider::activate(int index) {
    auto next = m_state.value();
    if (next.active == index) return true;
    next.active = next.focused     = index;
    const auto            sequence = m_sequence;
    QPointer<RangeSlider> guard(this);
    publish(next);
    if (! guard || m_sequence != sequence) return false;
    m_started = true;
    Q_EMIT interactionStarted(index);
    return guard && m_sequence == sequence;
}
int RangeSlider::pickHandle(qreal p, bool release) const {
    if (activeHandle() >= 0) return activeHandle();
    const qreal a = nodePosition(0), b = nodePosition(1);
    const qreal da = std::abs(p - a), db = std::abs(p - b);
    const qreal slop = QGuiApplication::styleHints()->startDragDistance();
    if (std::abs(a - b) * trackLength() <= slop &&
        std::max(std::abs(m_pressPosition - a), std::abs(m_pressPosition - b)) * trackLength() <=
            slop) {
        if (std::abs(p - m_pressPosition) * trackLength() <= slop)
            return release ? focusedHandle() : -1;
        return p < m_pressPosition ? 0 : 1;
    }
    return da == db ? focusedHandle() : da < db ? 0 : 1;
}
void RangeSlider::moveTo(qreal p, bool release) {
    const int index = pickHandle(p, release);
    if (index < 0) return;
    if (! activate(index)) return;
    auto        next        = m_state.value();
    const qreal length      = span(next);
    const qreal other       = distance(next, next.values[1 - index]);
    const qreal lower       = index == 0 ? 0 : std::min(length, other + next.effective);
    const qreal upper       = index == 0 ? std::max(qreal(0), other - next.effective) : length;
    const qreal continuous  = std::clamp(p * length, lower, upper);
    const qreal snapped     = nearest(next, continuous, lower, upper);
    const qreal oldPosition = next.positions[index];
    const qreal oldValue    = next.values[index];
    if (next.live || release) next.values[index] = value(next, snapped);
    const bool snap                = release || next.snap == SnapAlways || m_input == Input::Key;
    next.positions[index]          = length > 0 ? (snap ? snapped : continuous) / length : 0;
    const auto            sequence = m_sequence;
    QPointer<RangeSlider> guard(this);
    publish(next);
    if (! guard || m_sequence != sequence) return;
    if (oldPosition != next.positions[index] || oldValue != next.values[index])
        Q_EMIT node(index)->moved();
    if (! guard || m_sequence != sequence) return;
    if (release) finish(false);
}
void RangeSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || m_input != Input::None ||
        event->source() == Qt::MouseEventSynthesizedByQt) {
        event->ignore();
        return;
    }
    event->accept();
    m_input                        = Input::Mouse;
    m_pressPoint                   = event->position();
    m_pressPosition                = positionAt(m_pressPoint);
    const auto            sequence = ++m_sequence;
    QPointer<RangeSlider> guard(this);
    if (focusPolicy() & Qt::ClickFocus) forceActiveFocus(Qt::MouseFocusReason);
    if (! guard || m_sequence != sequence) return;
    setKeepMouseGrab(true);
    moveTo(m_pressPosition, false);
}
void RangeSlider::mouseMoveEvent(QMouseEvent* event) {
    event->setAccepted(m_input == Input::Mouse);
    if (m_input == Input::Mouse) moveTo(positionAt(event->position()), false);
}
void RangeSlider::mouseReleaseEvent(QMouseEvent* event) {
    const bool accepted = m_input == Input::Mouse && event->button() == Qt::LeftButton;
    event->setAccepted(accepted);
    if (accepted) moveTo(positionAt(event->position()), true);
}
void RangeSlider::mouseUngrabEvent() {
    if (m_input == Input::Mouse) finish(true);
}
void RangeSlider::touchUngrabEvent() {
    if (m_input == Input::Touch) finish(true);
}
void RangeSlider::touchEvent(QTouchEvent* event) {
    if (event->type() == QEvent::TouchCancel) {
        event->accept();
        if (m_input == Input::Touch) finish(true);
        return;
    }
    if (m_input == Input::None) {
        for (const auto& point : event->points()) {
            if (point.state() != QEventPoint::Pressed || ! contains(point.position())) continue;
            m_input = Input::Touch;
            ++m_sequence;
            m_touchId       = point.id();
            m_pressPoint    = point.position();
            m_pressPosition = positionAt(m_pressPoint);
            event->accept();
            return;
        }
    }
    event->setAccepted(m_input == Input::Touch);
    if (m_input != Input::Touch) return;
    for (const auto& point : event->points()) {
        if (point.id() != m_touchId) continue;
        const bool release = point.state() == QEventPoint::Released;
        if (! release && point.state() != QEventPoint::Updated) return;
        if (! keepTouchGrab() && ! release) {
            const QPointF delta     = point.position() - m_pressPoint;
            const qreal   primary   = std::abs(horizontal() ? delta.x() : delta.y());
            const qreal   cross     = std::abs(horizontal() ? delta.y() : delta.x());
            const qreal   threshold = QGuiApplication::styleHints()->startDragDistance();
            if (cross > threshold && cross > primary) {
                event->ignore();
                finish(true);
                return;
            }
            if (primary <= threshold) return;
            setKeepTouchGrab(true);
        }
        const auto            sequence = m_sequence;
        QPointer<RangeSlider> guard(this);
        if (focusPolicy() & Qt::ClickFocus) forceActiveFocus(Qt::MouseFocusReason);
        if (guard && m_sequence == sequence) moveTo(positionAt(point.position()), release);
        return;
    }
}
void RangeSlider::keyPressEvent(QKeyEvent* event) {
    const int key = event->key();
    if (key == Qt::Key_Escape && m_input != Input::None) {
        event->accept();
        finish(true);
        return;
    }
    if (m_input != Input::None && m_input != Input::Key) {
        event->ignore();
        return;
    }
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            event->ignore();
            return;
        }
        const bool backwards =
            key == Qt::Key_Backtab || event->modifiers().testFlag(Qt::ShiftModifier);
        const int index = focusedHandle() + (backwards ? -1 : 1);
        event->setAccepted(index >= 0 && index <= 1);
        if (event->isAccepted()) setFocusedHandle(index);
        return;
    }
    const bool arrow = horizontal() ? key == Qt::Key_Left || key == Qt::Key_Right
                                    : key == Qt::Key_Up || key == Qt::Key_Down;
    if (! arrow && key != Qt::Key_Home && key != Qt::Key_End) {
        event->ignore();
        return;
    }
    event->accept();
    if (m_input == Input::None) {
        m_input = Input::Key;
        ++m_sequence;
    }
    m_key = key;
    if (! activate(focusedHandle())) return;
    const auto state = m_state.value();
    qreal      p     = state.positions[state.focused];
    if (key == Qt::Key_Home || key == Qt::Key_End)
        p = key == Qt::Key_Home ? 0 : 1;
    else if (span(state) > 0) {
        const bool forward =
            horizontal() ? (key == Qt::Key_Right) != mirrored() : key == Qt::Key_Up;
        p += (forward ? 1 : -1) * (state.step > 0 ? state.step : .1) / span(state);
    }
    moveTo(p, false);
}
void RangeSlider::keyReleaseEvent(QKeyEvent* event) {
    const bool accepted = m_input == Input::Key && event->key() == m_key;
    event->setAccepted(accepted);
    if (accepted && ! event->isAutoRepeat()) moveTo(nodePosition(activeHandle()), true);
}
void RangeSlider::wheelEvent(QWheelEvent* event) {
    if (! wheelEnabled() || m_input != Input::None || event->angleDelta().isNull()) {
        event->ignore();
        return;
    }
    event->accept();
    const QPoint angle = event->angleDelta();
    const qreal  delta = (std::abs(angle.y()) >= std::abs(angle.x()) ? angle.y() : angle.x()) *
                         (event->inverted() ? -1 : 1) / 120.;
    m_input            = Input::Key;
    ++m_sequence;
    if (! activate(focusedHandle())) return;
    const auto state = m_state.value();
    moveTo(state.positions[state.focused] +
               (span(state) > 0 ? delta * (state.step > 0 ? state.step : .1) / span(state) : 0),
           true);
}
void RangeSlider::focusOutEvent(QFocusEvent* event) {
    QPointer<RangeSlider> guard(this);
    Control::focusOutEvent(event);
    if (guard && m_input == Input::Key) finish(true);
}
void RangeSlider::focusInEvent(QFocusEvent* event) {
    QPointer<RangeSlider> guard(this);
    Control::focusInEvent(event);
    if (! guard) return;
    if (event->reason() == Qt::TabFocusReason)
        setFocusedHandle(0);
    else if (event->reason() == Qt::BacktabFocusReason)
        setFocusedHandle(1);
}
void RangeSlider::observeWindow() {
    disconnect(m_windowConnection);
    if (auto win = window())
        m_windowConnection = connect(win, &QWindow::activeChanged, this, [this, win] {
            if (! win->isActive()) finish(true);
        });
}
void RangeSlider::itemChange(ItemChange change, const ItemChangeData& data) {
    QPointer<RangeSlider> guard(this);
    Control::itemChange(change, data);
    if (! guard) return;
    if (change == ItemSceneChange) observeWindow();
    if (change == ItemSceneChange || change == ItemParentHasChanged ||
        (change == ItemEnabledHasChanged && ! data.boolValue) ||
        (change == ItemVisibleHasChanged && ! data.boolValue))
        finish(true);
}
} // namespace qml_material
