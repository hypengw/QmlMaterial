#include "qml_material/input/input_host.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "control_accessibility_p.hpp"
#include <QFocusEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTouchEvent>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace qml_material
{
namespace
{
constexpr qint64        MinimumDoubleTapInterval = 40;
std::unique_ptr<QTimer> timer(QObject* context, std::function<void()> timeout) {
    auto result = std::make_unique<QTimer>();
    result->setSingleShot(true);
    QObject::connect(result.get(), &QTimer::timeout, context, std::move(timeout));
    return result;
}
} // namespace

class QtDragInputAdapter : public QQuickPointerHandler {
public:
    explicit QtDragInputAdapter(InputHost& host): QQuickPointerHandler(nullptr), m_host(host) {
        setParent(host.item());
        QQuickItemPrivate::get(host.item())->QQuickItemPrivate::addPointerHandler(this);
        setTarget(nullptr);
        setGrabPermissions(CanTakeOverFromItems | ApprovesCancellation);
        host.drag().requestGrab = [this] {
            if (! m_event) return false;
            auto& point   = m_event->point(m_pointIndex);
            auto* grabber = qobject_cast<QQuickItem*>(m_event->exclusiveGrabber(point));
            if (grabber && grabber != m_host.item() &&
                (grabber->keepMouseGrab() || grabber->keepTouchGrab()))
                return false;
            return setExclusiveGrab(m_event, point);
        };
    }
    ~QtDragInputAdapter() override { detach(); }
    void detach() {
        const auto device = std::exchange(m_device, nullptr);
        if (device)
            QPointingDevicePrivate::get(const_cast<QPointingDevice*>(device.data()))
                ->removeGrabber(this, true);
    }

protected:
    bool wantsPointerEvent(QPointerEvent* event) override {
        if (event->type() == QEvent::TouchCancel) {
            if (m_device) m_host.cancelActivation();
            return false;
        }
        if (! m_host.available()) return false;
        const auto type = event->type();
        if (type != QEvent::TouchBegin && type != QEvent::TouchUpdate && type != QEvent::TouchEnd &&
            type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease &&
            type != QEvent::MouseMove)
            return false;
        if (auto* mouse = dynamic_cast<QMouseEvent*>(event);
            mouse && mouse->button() != Qt::LeftButton && mouse->buttons() != Qt::LeftButton &&
            type != QEvent::MouseMove)
            return false;
        if (m_device && m_device != event->pointingDevice()) return false;
        for (int i = 0; i < event->pointCount(); ++i) {
            const auto& point = event->point(i);
            if (m_device ? point.id() == m_identity
                         : point.state() == QEventPoint::Pressed &&
                               m_host.item()->contains(
                                   m_host.item()->mapFromScene(point.scenePosition()))) {
                m_pointIndex = i;
                return true;
            }
        }
        return false;
    }
    void handlePointerEventImpl(QPointerEvent* event) override {
        const QPointer<QtDragInputAdapter> guard(this);
        const auto&                        point = event->point(m_pointIndex);
        const auto  position = m_host.item()->mapFromScene(point.scenePosition());
        const auto* device   = event->pointingDevice();
        const auto  kind     = device->type() == QInputDevice::DeviceType::TouchScreen
                                   ? Interaction::Touch
                               : device->pointerType() == QPointingDevice::PointerType::Pen ||
                                       device->pointerType() == QPointingDevice::PointerType::Eraser
                                   ? Interaction::Pen
                                   : Interaction::Mouse;
        const PointerInput input { kind, point.id(), position, qint64(event->timestamp()) };
        m_event = event;
        if (point.state() == QEventPoint::Pressed) {
            m_device   = event->pointingDevice();
            m_identity = point.id();
            setPassiveGrab(event, point);
            m_host.click().begin(input);
        } else {
            if (m_host.m_drag)
                m_host.m_drag->velocity = m_host.item()->mapFromScene(point.velocity().toPointF()) -
                                          m_host.item()->mapFromScene(QPointF());
            const bool released   = point.state() == QEventPoint::Released;
            const auto generation = m_host.m_generation;
            m_host.click().move(input, ! released);
            if (! guard) return;
            if (released && generation == m_host.m_generation) {
                m_host.click().finish(input);
                if (! guard) return;
                detach();
            }
        }
        if (guard) m_event = nullptr;
    }
    void onGrabChanged(QQuickPointerHandler* grabber, QPointingDevice::GrabTransition transition,
                       QPointerEvent* event, QEventPoint& point) override {
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        if (m_device && grabber == this &&
            (transition == QPointingDevice::CancelGrabExclusive ||
             transition == QPointingDevice::CancelGrabPassive))
            m_host.cancelActivation();
    }

private:
    InputHost&                      m_host;
    QPointer<const QPointingDevice> m_device;
    int                             m_identity   = -1;
    int                             m_pointIndex = 0;
    QPointerEvent*                  m_event      = nullptr;
};

InputHost::InputHost(QQuickItem* item, std::function<void()> changed)
    : m_item(item), m_sourceChanged(std::move(changed)) {
    Q_ASSERT(item);
    detail::ensureControlAccessibility();
    m_elapsed.start();
    m_connections.append(QObject::connect(item, &QObject::destroyed, item, [this] {
        const std::weak_ptr<const bool> alive(m_lifetime);
        m_detaching = true;
        m_pointerAdapter.reset();
        QObject::disconnect(m_sourceDestroyed);
        m_emitter.reset();
        m_source   = nullptr;
        auto owned = std::move(m_ownedSource);
        owned.reset();
        if (! alive.expired()) reset(true);
    }));
    m_connections.append(QObject::connect(item, &QQuickItem::enabledChanged, item, [this] {
        if (! available()) reset(true);
    }));
    m_connections.append(QObject::connect(item, &QQuickItem::visibleChanged, item, [this] {
        if (! available()) reset(true);
    }));
    m_connections.append(QObject::connect(item, &QQuickItem::parentChanged, item, [this] {
        reset(true);
    }));
    m_connections.append(QObject::connect(item, &QQuickItem::activeFocusChanged, item, [this] {
        focus();
    }));
    m_connections.append(QObject::connect(item, &QQuickItem::windowChanged, item, [this] {
        QPointer<QQuickItem>            guard(m_item);
        const std::weak_ptr<const bool> alive(m_lifetime);
        reset(true);
        if (! alive.expired() && guard) observeWindow();
    }));
    observeWindow();
}
InputHost::~InputHost() {
    m_lifetime.reset();
    m_detaching = true;
    m_pointerAdapter.reset();
    for (const auto& connection : m_connections) QObject::disconnect(connection);
    QObject::disconnect(m_windowActive);
    QObject::disconnect(m_windowVisibility);
    QObject::disconnect(m_sourceDestroyed);
    reset(false);
}
bool InputHost::available() const {
    return ! m_detaching && m_item && m_item->isEnabled() && m_item->isVisible() &&
           (! acceptsInput || acceptsInput());
}
void InputHost::observeWindow() {
    QObject::disconnect(m_windowActive);
    QObject::disconnect(m_windowVisibility);
    if (auto* window = m_item->window()) {
        m_windowActive = QObject::connect(window, &QWindow::activeChanged, m_item, [this, window] {
            if (! window->isActive()) reset(true);
        });
        m_windowVisibility =
            QObject::connect(window, &QWindow::visibleChanged, m_item, [this](bool visible) {
                if (! visible) reset(true);
            });
    }
}
InteractionSource* InputHost::source() {
    if (! m_source && ! m_detaching) {
        m_ownedSource = std::make_unique<InteractionSource>(m_item);
        attachSource(m_ownedSource.get());
    }
    return m_source;
}
void InputHost::attachSource(InteractionSource* source) {
    m_source  = source;
    m_emitter = source ? source->emitter(m_item) : nullptr;
    if (source)
        m_sourceDestroyed = QObject::connect(source, &QObject::destroyed, m_item, [this, source] {
            if (m_ownedSource.get() == source) m_ownedSource.release();
            m_emitter.reset();
            QPointer<QQuickItem>            guard(m_item);
            const std::weak_ptr<const bool> alive(m_lifetime);
            reset(true);
            if (alive.expired() || ! guard) return;
            const auto notify = m_sourceChanged;
            if (notify) notify();
        });
}
void InputHost::setSource(InteractionSource* source) {
    if (source == m_source) return;
    QPointer<QQuickItem>            guard(m_item);
    const std::weak_ptr<const bool> alive(m_lifetime);
    QPointer<InteractionSource>     target(source);
    const auto                      generation = m_generation + 1;
    reset(true);
    if (alive.expired() || ! guard || generation != m_generation) return;
    QObject::disconnect(m_sourceDestroyed);
    m_emitter.reset();
    m_source = nullptr;
    m_ownedSource.reset();
    if (alive.expired() || ! guard || generation != m_generation) return;
    attachSource(target);
    const auto notify = m_sourceChanged;
    if (notify) notify();
}
quint64 InputHost::begin(Interaction::Kind kind, const PointerInput& input) {
    return available() && m_emitter
               ? m_emitter->begin(kind, input.device, input.identity, input.position)
               : 0;
}
void InputHost::end(quint64 token, bool cancelled) {
    if (m_emitter) m_emitter->end(token, cancelled ? Interaction::Cancel : Interaction::End);
}
void InputHost::hover(bool active) {
    if (active && available()) {
        if (! m_hover.token) m_hover.token = begin(Interaction::Hover);
    } else
        end(std::exchange(m_hover.token, 0));
}
void InputHost::setHoverEnabled(bool enabled) {
    m_item->setAcceptHoverEvents(enabled);
    if (! enabled) hover(false);
}
void InputHost::observeHover(bool value) { hover(value && m_item->acceptHoverEvents()); }
void InputHost::setFocusEnabled(bool value) {
    if (m_focusEnabled == value) return;
    m_focusEnabled = value;
    const QPointer<QQuickItem> guard(m_item);
    focus();
    if (guard) focusReason(m_focusReason);
}
void InputHost::focus() {
    if (m_focusEnabled && available() && m_item->hasActiveFocus()) {
        if (! m_focus.token) m_focus.token = begin(Interaction::Focus);
    } else {
        end(std::exchange(m_focus.token, 0));
        if (m_click) {
            if (! m_click->m_keys.isEmpty()) ++m_generation;
            m_click->cancelKeys();
        }
    }
}
void InputHost::focusReason(Qt::FocusReason reason) {
    m_focusReason      = reason;
    const bool visible = m_focusEnabled && available() && m_item->hasActiveFocus() &&
                         (reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
                          reason == Qt::ShortcutFocusReason);
    if (visible == m_focusVisible) return;
    m_focusVisible    = visible;
    const auto notify = focusVisibleChanged;
    if (notify) notify();
}
void InputHost::blur() {
    end(std::exchange(m_focus.token, 0));
    if (m_click) {
        if (! m_click->m_keys.isEmpty()) ++m_generation;
        m_click->cancelKeys();
    }
    focusReason(Qt::OtherFocusReason);
}
ClickInput& InputHost::click() {
    if (! m_click) m_click.emplace(*this);
    return *m_click;
}
DragInput& InputHost::drag() {
    click();
    if (! m_drag) m_drag = std::make_unique<DragInput>();
    return *m_drag;
}
void InputHost::resetDrag() {
    if (! m_drag) return;
    QPointer<QQuickItem>            guard(m_item);
    const std::weak_ptr<const bool> alive(m_lifetime);
    const auto                      generation = m_generation + 1;
    reset(true);
    if (! alive.expired() && guard && generation == m_generation) {
        m_pointerAdapter.reset();
        m_drag.reset();
    }
}
void InputHost::reset(bool notify) {
    ++m_generation;
    if (m_pointerAdapter) m_pointerAdapter->detach();
    m_hover.token = m_focus.token = m_editorPress = 0;
    if (m_emitter) m_emitter->cancel();
    if (m_click) m_click->cancel(notify);
}
void InputHost::cancel() { reset(true); }
void InputHost::cancelActivation() {
    // Retarget activation without ending the physical item's hover or focus.
    ++m_generation;
    if (m_pointerAdapter) m_pointerAdapter->detach();
    if (m_click) m_click->cancel(true);
}
void InputHost::observeChildPointerInput() {
    if (! m_pointerAdapter) m_pointerAdapter = std::make_unique<QtDragInputAdapter>(*this);
}
bool InputHost::event(QEvent* event) {
    if (m_pointerAdapter) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::MouseButtonDblClick:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel: return false;
        default: break;
        }
    }
    return QtInputAdapter::deliver(*this, event);
}
bool InputHost::observeEvent(QEvent* event) {
    // Pointer/key virtual handlers run after Qt's focus and Keys filters.
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::UngrabMouse: return this->event(event);
    default: return false;
    }
}
void InputHost::mouseUngrabbed() {
    ++m_generation;
    end(std::exchange(m_editorPress, 0), true);
    if (m_click && m_click->m_pointer && m_click->m_pointer->input.device != Interaction::Touch)
        m_click->cancelPointer(true);
}
void InputHost::touchUngrabbed() {
    ++m_generation;
    end(std::exchange(m_editorPress, 0), true);
    if (m_click && m_click->m_pointer && m_click->m_pointer->input.device == Interaction::Touch)
        m_click->cancelPointer(true);
}

ClickInput::ClickInput(InputHost& host): m_host(host) {}
ClickInput::~ClickInput() = default;
CombinedClickInput& ClickInput::combined() {
    if (! m_combined) m_combined = std::make_unique<CombinedClickInput>();
    return *m_combined;
}
RepeatClickInput& ClickInput::repeat() {
    if (! m_repeat) m_repeat = std::make_unique<RepeatClickInput>();
    return *m_repeat;
}
void ClickInput::resetCombined() {
    cancel(false);
    m_combined.reset();
}
void ClickInput::activate() {
    const auto notify = clicked;
    if (m_host.available() && notify) notify();
}
quint64 ClickInput::tapIdentity(const Candidate& press) {
    return press.input.device == Interaction::Keyboard
               ? (quint64(Interaction::Keyboard) << 32) | quint32(press.input.identity)
               : 0;
}
bool ClickInput::prepare(Candidate& press) {
    if (m_repeat) press.repeatDeadline = m_host.m_elapsed.elapsed() + std::max(1, m_repeat->delay);
    if (! m_combined) return false;
    press.deadline = m_combined->longClickEnabled
                         ? m_host.m_elapsed.elapsed() +
                               (m_combined->longClickInterval > 0
                                    ? m_combined->longClickInterval
                                    : QGuiApplication::styleHints()->mousePressAndHoldInterval())
                         : -1;
    auto tap       = m_combined->taps.find(tapIdentity(press));
    if (tap == m_combined->taps.end()) return false;
    if (press.input.timestamp - tap->released >
        QGuiApplication::styleHints()->mouseDoubleClickInterval()) {
        m_combined->taps.erase(tap);
        return true;
    }
    if (press.input.timestamp - tap->released < MinimumDoubleTapInterval) {
        if (press.input.device != Interaction::Keyboard) {
            press.ignored  = true;
            press.deadline = -1;
            return false;
        }
        m_combined->taps.erase(tap);
        return true;
    }
    press.second = true;
    m_combined->taps.erase(tap);
    return false;
}
void ClickInput::feedback(Candidate& press) {
    if (press.feedbackEligible && ! press.token && ! press.ignored && ! press.dragging)
        press.token = m_host.begin(Interaction::Press, press.input);
}
void ClickInput::feedbackTimeout() {
    if (m_pointer) feedback(*m_pointer);
}
void ClickInput::begin(const PointerInput& input) {
    if (! m_host.available() || m_pointer) return;
    ++m_host.m_generation;
    m_pointer                   = Candidate { input };
    m_pointer->feedbackEligible = bool(m_host.existingSource());
    const bool previousTap      = prepare(*m_pointer);
    if (m_pointer->ignored) return;
    const auto                      generation = m_host.m_generation;
    QPointer<QQuickItem>            guard(m_host.item());
    const std::weak_ptr<const bool> alive(m_host.m_lifetime);
    const auto                      started = pointerStarted;
    if (started) started(input);
    if (alive.expired() || ! guard || generation != m_host.m_generation || ! m_pointer) return;
    if (m_host.m_drag && m_host.m_drag->startImmediately) {
        const auto canStart = m_host.m_drag->canStart;
        const bool accepted = ! canStart || canStart(input.position, input.position);
        if (alive.expired() || ! guard || generation != m_host.m_generation || ! m_pointer) return;
        if (accepted) {
            if (startDrag()) {
                const auto moved = m_host.m_drag->moved;
                if (moved) moved(input.position);
            }
            return;
        }
    }
    const int delay = NestedScroll::childPressDelay(m_host.item());
    if (delay > 0 && m_host.existingSource()) {
        if (! m_feedbackTimer)
            m_feedbackTimer = timer(m_host.item(), [this] {
                feedbackTimeout();
            });
        m_feedbackTimer->start(delay);
    } else
        feedback(*m_pointer);
    scheduleHold();
    scheduleRepeat();
    if (previousTap) activate();
}
bool ClickInput::startDrag() {
    auto* drag = m_host.m_drag.get();
    if (! drag || ! m_pointer) return false;
    const QPointer<QQuickItem>      alive(m_host.item());
    const std::weak_ptr<const bool> lifetime(m_host.m_lifetime);
    const auto                      revision = m_host.m_generation;
    if (drag->requestGrab) {
        const bool accepted = drag->requestGrab();
        if (lifetime.expired() || ! alive || revision != m_host.m_generation) return false;
        if (! accepted) {
            cancelPointer(true);
            return false;
        }
    }
    if (m_feedbackTimer) m_feedbackTimer->stop();
    m_host.end(std::exchange(m_pointer->token, 0), true);
    m_pointer->dragging       = true;
    m_pointer->deadline       = -1;
    m_pointer->repeatDeadline = -1;
    drag->token               = m_host.begin(Interaction::Drag, m_pointer->input);
    m_host.item()->setKeepMouseGrab(true);
    m_host.item()->setKeepTouchGrab(true);
    scheduleHold();
    scheduleRepeat();
    const auto           callback = drag->started;
    const auto           origin   = m_pointer->input.position;
    QPointer<QQuickItem> guard(m_host.item());
    const auto           generation = m_host.m_generation;
    if (callback) callback(origin);
    return ! lifetime.expired() && guard && generation == m_host.m_generation && m_pointer &&
           m_pointer->dragging;
}
void ClickInput::move(const PointerInput& input, bool notifyDrag) {
    if (! m_pointer || m_pointer->ignored) return;
    auto* drag = m_host.m_drag.get();
    if (drag && ! m_pointer->held) {
        if (! m_pointer->dragging) {
            const auto  delta     = input.position - m_pointer->input.position;
            const auto  threshold = drag->threshold >= 0
                                        ? drag->threshold
                                        : QGuiApplication::styleHints()->startDragDistance();
            const auto  x = std::abs(delta.x()), y = std::abs(delta.y());
            const bool  planar  = drag->axes == (Qt::Horizontal | Qt::Vertical);
            const qreal primary = planar ? std::hypot(x, y) : drag->axes == Qt::Horizontal ? x : y;
            const qreal cross   = drag->axes == Qt::Horizontal ? y : x;
            if (! planar && cross > threshold && cross > primary) {
                cancelPointer(true);
                return;
            }
            const bool crossed =
                drag->axes && primary > 0 && (drag->startImmediately || primary >= threshold);
            if (crossed) {
                const auto canStart = drag->canStart;
                bool       accepted = true;
                if (canStart) {
                    const QPointer<QQuickItem>      guard(m_host.item());
                    const std::weak_ptr<const bool> alive(m_host.m_lifetime);
                    const auto                      generation = m_host.m_generation;
                    accepted = canStart(m_pointer->input.position, input.position);
                    if (alive.expired() || ! guard || generation != m_host.m_generation) return;
                }
                if (accepted && ! startDrag()) return;
            }
        }
        if (m_pointer && m_pointer->dragging) {
            const auto callback = drag->moved;
            if (callback && notifyDrag) callback(input.position);
            return;
        }
    }
    if (m_pointer && (! m_pointer->held || m_repeat) && ! m_host.item()->contains(input.position))
        cancelPointer(true);
    else if (m_pointer && notifyDrag) {
        const auto callback = pointerMoved;
        if (callback) callback(input.position);
    }
}
void ClickInput::release(Candidate& press, bool cancelled) {
    if (press.ignored || press.dragging) return;
    if (! cancelled) feedback(press);
    m_host.end(press.token, cancelled);
}
void ClickInput::finish(const PointerInput& input) {
    if (! m_pointer) return;
    auto press            = *m_pointer;
    press.input.timestamp = input.timestamp;
    press.input.position  = input.position;
    m_pointer.reset();
    if (m_feedbackTimer) m_feedbackTimer->stop();
    const bool cancelled = ! m_host.available();
    release(press, cancelled);
    scheduleHold();
    scheduleRepeat();
    if (press.dragging && m_host.m_drag) {
        auto* drag = m_host.m_drag.get();
        m_host.end(std::exchange(drag->token, 0), cancelled);
        if (auto* item = m_host.item()) {
            item->setKeepMouseGrab(false);
            item->setKeepTouchGrab(false);
        }
        const auto callback = drag->finished;
        if (callback) callback(input.position, cancelled);
    } else if (! cancelled && ! press.held && ! press.ignored)
        complete(press);
    else if (cancelled) {
        const auto callback = pointerCancelled;
        if (callback) callback();
    }
}
void ClickInput::complete(const Candidate& press) {
    if (m_combined && m_combined->doubleClickEnabled) {
        if (press.second) {
            const auto callback = m_combined->doubleClicked;
            if (callback) callback();
        } else {
            m_combined->taps.insert(
                tapIdentity(press),
                { press.input.timestamp,
                  m_host.m_elapsed.elapsed() +
                      QGuiApplication::styleHints()->mouseDoubleClickInterval() });
            scheduleTap();
        }
    } else if (press.input.device != Interaction::Keyboard && pointerClicked) {
        const auto callback = pointerClicked;
        callback(press.input.position);
    } else
        activate();
}
void ClickInput::cancelPointer(bool notify) {
    if (! m_pointer) return;
    auto press = *m_pointer;
    m_pointer.reset();
    if (m_feedbackTimer) m_feedbackTimer->stop();
    if (m_combined) m_combined->taps.remove(tapIdentity(press));
    release(press, true);
    scheduleHold();
    scheduleRepeat();
    scheduleTap();
    if (press.dragging && m_host.m_drag) {
        auto* drag = m_host.m_drag.get();
        m_host.end(std::exchange(drag->token, 0), true);
        if (auto* item = m_host.item()) {
            item->setKeepMouseGrab(false);
            item->setKeepTouchGrab(false);
        }
        const auto callback = drag->finished;
        if (notify && callback) callback(press.input.position, true);
    } else {
        const auto callback = pointerCancelled;
        if (notify && callback) callback();
    }
}
void ClickInput::cancelKeys() {
    auto keys = std::exchange(m_keys, {});
    for (auto& press : keys) release(press, true);
    if (m_combined) {
        for (auto it = m_combined->taps.begin(); it != m_combined->taps.end();)
            if ((it.key() >> 32) == Interaction::Keyboard)
                it = m_combined->taps.erase(it);
            else
                ++it;
    }
    scheduleHold();
    scheduleRepeat();
    scheduleTap();
}
void ClickInput::cancel(bool notify) {
    cancelKeys();
    if (m_combined) {
        m_combined->taps.clear();
        if (m_combined->tapTimer) m_combined->tapTimer->stop();
    }
    cancelPointer(notify);
}
bool ClickInput::key(int key, bool down, bool repeat, qint64 timestamp) {
    if (! keyboardEnabled) return false;
    if (key != Qt::Key_Space && key != Qt::Key_Return && key != Qt::Key_Enter &&
        key != Qt::Key_Select)
        return false;
    if (! m_host.available()) return false;
    if (repeat) return true;
    if (down) {
        if (m_keys.contains(key)) return true;
        Candidate press { { Interaction::Keyboard,
                            key,
                            { m_host.item()->width() / 2, m_host.item()->height() / 2 },
                            timestamp } };
        press.feedbackEligible = bool(m_host.existingSource());
        const bool previousTap = prepare(press);
        feedback(press);
        m_keys.insert(key, press);
        scheduleHold();
        scheduleRepeat();
        if (previousTap) activate();
    } else {
        if (! m_keys.contains(key)) return false;
        auto press            = m_keys.take(key);
        press.input.timestamp = timestamp;
        release(press, false);
        scheduleHold();
        scheduleRepeat();
        if (! press.held && ! press.ignored) complete(press);
    }
    return true;
}
void ClickInput::scheduleHold() {
    if (! m_combined) return;
    qint64     deadline = -1;
    const auto consider = [&](const Candidate& press) {
        if (press.deadline >= 0 && (deadline < 0 || press.deadline < deadline))
            deadline = press.deadline;
    };
    if (m_pointer) consider(*m_pointer);
    for (const auto& press : m_keys) consider(press);
    if (deadline < 0) {
        if (m_combined->holdTimer) m_combined->holdTimer->stop();
    } else {
        if (! m_combined->holdTimer)
            m_combined->holdTimer = timer(m_host.item(), [this] {
                holdTimeout();
            });
        m_combined->holdTimer->start(
            int(std::max(qint64(0), deadline - m_host.m_elapsed.elapsed())));
    }
}
void ClickInput::scheduleRepeat() {
    if (! m_repeat) return;
    qint64     deadline = -1;
    const auto consider = [&](const Candidate& press) {
        if (press.repeatDeadline >= 0 && (deadline < 0 || press.repeatDeadline < deadline))
            deadline = press.repeatDeadline;
    };
    if (m_pointer) consider(*m_pointer);
    for (const auto& press : m_keys) consider(press);
    if (deadline < 0) {
        if (m_repeat->timer) m_repeat->timer->stop();
        return;
    }
    if (! m_repeat->timer)
        m_repeat->timer = timer(m_host.item(), [this] {
            repeatTimeout();
        });
    m_repeat->timer->start(int(std::max(qint64(0), deadline - m_host.m_elapsed.elapsed())));
}
void ClickInput::repeatTimeout() {
    const auto now      = m_host.m_elapsed.elapsed();
    int        count    = 0;
    const auto consider = [&](Candidate& press) {
        if (press.repeatDeadline >= 0 && press.repeatDeadline <= now) {
            press.repeatDeadline = now + std::max(1, m_repeat->interval);
            press.deadline       = -1;
            press.held           = true;
            ++count;
        }
    };
    if (m_pointer) consider(*m_pointer);
    for (auto& press : m_keys) consider(press);
    scheduleHold();
    scheduleRepeat();
    QPointer<QQuickItem>            guard(m_host.item());
    const std::weak_ptr<const bool> alive(m_host.m_lifetime);
    const auto                      generation = m_host.m_generation;
    for (int i = 0; i < count; ++i) {
        activate();
        if (alive.expired() || ! guard || generation != m_host.m_generation) return;
    }
}
void ClickInput::holdTimeout() {
    if (! m_combined) return;
    const auto now      = m_host.m_elapsed.elapsed();
    int        count    = 0;
    const auto consider = [&](Candidate& press) {
        if (press.deadline >= 0 && press.deadline <= now) {
            press.deadline       = -1;
            press.repeatDeadline = -1;
            press.held           = true;
            ++count;
        }
    };
    if (m_pointer) consider(*m_pointer);
    for (auto& press : m_keys) consider(press);
    scheduleHold();
    scheduleRepeat();
    const auto                      callback = m_combined->longClicked;
    QPointer<QQuickItem>            guard(m_host.item());
    const std::weak_ptr<const bool> alive(m_host.m_lifetime);
    const auto                      generation = m_host.m_generation;
    for (int i = 0; i < count; ++i) {
        if (callback && m_host.available()) callback();
        if (alive.expired() || ! guard || generation != m_host.m_generation) return;
    }
}
void ClickInput::scheduleTap() {
    if (! m_combined) return;
    if (m_combined->taps.isEmpty()) {
        if (m_combined->tapTimer) m_combined->tapTimer->stop();
        return;
    }
    qint64 deadline = std::numeric_limits<qint64>::max();
    for (const auto& tap : m_combined->taps) deadline = std::min(deadline, tap.deadline);
    if (! m_combined->tapTimer)
        m_combined->tapTimer = timer(m_host.item(), [this] {
            tapTimeout();
        });
    m_combined->tapTimer->start(int(std::max(qint64(0), deadline - m_host.m_elapsed.elapsed())));
}
void ClickInput::tapTimeout() {
    if (! m_combined) return;
    const auto     now = m_host.m_elapsed.elapsed();
    QList<quint64> ready;
    for (auto it = m_combined->taps.cbegin(); it != m_combined->taps.cend(); ++it)
        if (it->deadline <= now) ready.append(it.key());
    QPointer<QQuickItem>            guard(m_host.item());
    const std::weak_ptr<const bool> alive(m_host.m_lifetime);
    const auto                      generation = m_host.m_generation;
    for (auto id : ready) {
        m_combined->taps.remove(id);
        activate();
        if (alive.expired() || ! guard || generation != m_host.m_generation) return;
    }
    scheduleTap();
}

bool QtInputAdapter::deliverToEditor(InputHost& host, QEvent* event,
                                     const std::function<bool()>& receiver) {
    // QObject child events can arrive before the embedded host is constructed.
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::UngrabMouse:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::FocusIn:
    case QEvent::FocusOut: break;
    default: return receiver();
    }
    Q_ASSERT(! host.m_click || (! host.m_click->m_pointer && host.m_click->m_keys.isEmpty()));
    QPointer<QQuickItem> guard(host.item());
    const bool           handled = host.observeEvent(event);
    if (! guard || handled) return true;
    const auto type    = event->type();
    const bool pointer = type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick ||
                         type == QEvent::MouseMove || type == QEvent::MouseButtonRelease;
    const auto generation       = pointer ? ++host.m_generation : host.m_generation;
    const bool feedbackEligible = host.existingSource() && host.available();
    const bool result           = receiver();
    if (! guard || ! pointer || generation != host.m_generation) return result;
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick) {
        if (mouse->button() != Qt::LeftButton || ! mouse->isAccepted() || ! feedbackEligible)
            return result;
        host.end(std::exchange(host.m_editorPress, 0), true);
        const auto* device = mouse->pointingDevice();
        const auto  kind   = device->type() == QInputDevice::DeviceType::TouchScreen
                                 ? Interaction::Touch
                             : device->pointerType() == QPointingDevice::PointerType::Pen ||
                                     device->pointerType() == QPointingDevice::PointerType::Eraser
                                 ? Interaction::Pen
                                 : Interaction::Mouse;
        host.m_editorPress = host.begin(
            Interaction::Press,
            { kind, mouse->point(0).id(), mouse->position(), qint64(mouse->timestamp()) });
    } else if (type == QEvent::MouseMove) {
        if (! host.available() || host.item()->keepMouseGrab() ||
            ! host.item()->contains(mouse->position()))
            host.end(std::exchange(host.m_editorPress, 0), true);
    } else if (mouse->button() == Qt::LeftButton) {
        const bool cancelled = ! host.available() || ! host.item()->contains(mouse->position());
        host.end(std::exchange(host.m_editorPress, 0), cancelled);
    }
    return result;
}

bool QtInputAdapter::deliver(InputHost& host, QEvent* event) {
    const auto timestamp = [&](QInputEvent* event) {
        return event->timestamp() ? qint64(event->timestamp()) : host.m_elapsed.elapsed();
    };
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        if (! host.m_click) return false;
        auto&      click = *host.m_click;
        auto*      mouse = static_cast<QMouseEvent*>(event);
        const auto device =
            mouse->pointingDevice()->pointerType() == QPointingDevice::PointerType::Pen
                ? Interaction::Pen
                : Interaction::Mouse;
        const PointerInput input { device, -1, mouse->position(), timestamp(mouse) };
        if (mouse->source() == Qt::MouseEventSynthesizedByQt) {
            event->ignore();
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonDblClick) {
            const bool accepted = host.available() && mouse->button() == Qt::LeftButton &&
                                  ! click.m_pointer && host.item()->contains(input.position);
            event->setAccepted(accepted);
            if (accepted) click.begin(input);
        } else {
            const bool accepted =
                click.m_pointer && click.m_pointer->input.device == device &&
                (event->type() == QEvent::MouseMove || mouse->button() == Qt::LeftButton);
            event->setAccepted(accepted);
            if (! accepted) return true;
            QPointer<QQuickItem> guard(host.item());
            const auto           generation = host.m_generation;
            click.move(input, event->type() != QEvent::MouseButtonRelease);
            if (guard && generation == host.m_generation &&
                event->type() == QEvent::MouseButtonRelease)
                click.finish(input);
        }
        return true;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        if (! host.m_click) return false;
        auto& click = *host.m_click;
        auto* touch = static_cast<QTouchEvent*>(event);
        if (event->type() == QEvent::TouchCancel) {
            event->accept();
            if (click.m_pointer && click.m_pointer->input.device == Interaction::Touch)
                click.cancelPointer(true);
            return true;
        }
        if (! click.m_pointer) {
            for (const auto& point : touch->points()) {
                if (point.state() == QEventPoint::Pressed && host.available() &&
                    host.item()->contains(point.position())) {
                    event->accept();
                    click.begin(
                        { Interaction::Touch, point.id(), point.position(), timestamp(touch) });
                    return true;
                }
            }
            event->ignore();
            return true;
        }
        if (click.m_pointer->input.device != Interaction::Touch) {
            event->ignore();
            return true;
        }
        event->accept();
        bool    allUp = true, outside = false;
        QPointF position = click.m_pointer->input.position;
        for (const auto& point : touch->points()) {
            allUp &= point.state() == QEventPoint::Released;
            outside |= ! host.item()->contains(point.position());
            if (point.id() == click.m_pointer->input.identity) position = point.position();
        }
        const PointerInput input {
            Interaction::Touch, click.m_pointer->input.identity, position, timestamp(touch)
        };
        QPointer<QQuickItem> guard(host.item());
        const auto           generation = host.m_generation;
        click.move(input, ! allUp);
        if (! guard || generation != host.m_generation || ! click.m_pointer) return true;
        if (outside && ! click.m_pointer->held && ! click.m_pointer->dragging)
            click.cancelPointer(true);
        else if (allUp)
            click.finish(input);
        return true;
    }
    case QEvent::UngrabMouse: host.mouseUngrabbed(); return false;
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        if (! host.m_click) return false;
        auto&      click   = *host.m_click;
        auto*      key     = static_cast<QKeyEvent*>(event);
        const bool handled = click.key(
            key->key(), event->type() == QEvent::KeyPress, key->isAutoRepeat(), timestamp(key));
        event->setAccepted(handled);
        return handled;
    }
    case QEvent::HoverEnter: host.hover(host.item()->acceptHoverEvents()); break;
    case QEvent::HoverLeave: host.hover(false); break;
    case QEvent::FocusIn: host.focusReason(static_cast<QFocusEvent*>(event)->reason()); break;
    case QEvent::FocusOut: host.blur(); break;
    default: break;
    }
    return false;
}
} // namespace qml_material
