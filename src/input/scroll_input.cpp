#include "scroll_input_p.hpp"
#include "scroll_gesture_p.hpp"
#include "scroll_coordinates_p.hpp"
#include "nested_scroll_dispatcher_p.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "scroll_participant_p.hpp"
#include "qml_material/scrollable/scroll_motion.hpp"
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QStyleHints>
#include <QWheelEvent>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace qml_material
{
namespace
{
NestedScroll* attached(QQuickItem* item) {
    return qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(item, false));
}
QList<QPointer<NestedScroll>> chainFor(QQuickItem* item, bool wheel = false) {
    QList<QPointer<NestedScroll>> result;
    for (auto* p = item; p; p = p->parentItem()) {
        if (! ScrollParticipant(p).item()) continue;
        auto* config = attached(p);
        if (! config || ! ScrollParticipant(p).available()) break;
        const bool owned    = qobject_cast<Flickable*>(p);
        const bool ownFirst = result.isEmpty() && p == item && owned;
        if (! config->enabled() && ! (ownFirst || (wheel && owned))) break;
        result.append(config);
        if (! config->enabled() && ! wheel) break;
    }
    return result;
}

class ScrollFrameItem : public QQuickItem {
public:
    ScrollFrameItem(QQuickItem* parent, std::function<void()> tick)
        : QQuickItem(parent), m_tick(std::move(tick)) {}

protected:
    void updatePolish() override { m_tick(); }

private:
    std::function<void()> m_tick;
};
} // namespace

class ScrollInputHandler : public QQuickPointerHandler {
public:
    explicit ScrollInputHandler(NestedScroll* config)
        : QQuickPointerHandler(nullptr), m_config(config) {
        // Flickable redirects ordinary handlers to contentItem, where its own
        // child filter could intercept the coordinator's passive grab.
        setParent(config->item());
        QQuickItemPrivate::get(config->item())->QQuickItemPrivate::addPointerHandler(this);
        setTarget(nullptr);
        setGrabPermissions(CanTakeOverFromItems | ApprovesCancellation);
        m_frame = new ScrollFrameItem(config->item(), [this] {
            advance();
        });
        m_clock.start();
    }
    ~ScrollInputHandler() override {
        stop();
        delete m_frame;
    }

    void stop(bool cancelled = true) {
        if (m_stopping) return;
        ++m_revision;
        m_stopping = true;
        QPointer<ScrollInputHandler> guard(this);
        const bool                   started = std::exchange(m_started, false);
        m_pressed                            = false;
        m_dragging                           = false;
        m_gesture.clear();
        m_motionX.stop();
        m_motionY.stop();
        const auto chain     = std::exchange(m_chain, {});
        const auto behaviors = std::exchange(m_behaviors, {});
        for (const auto& c : std::exchange(m_connections, {})) disconnect(c);
        if (m_window) m_window->removeEventFilter(this);
        m_window = nullptr;
        if (m_device)
            QPointingDevicePrivate::get(const_cast<QPointingDevice*>(m_device.data()))
                ->removeGrabber(this, true);
        if (! guard) return;
        m_device = nullptr;
        restoreGrab();
        setActive(false);
        if (! guard) return;
        QList<QPointer<NestedScroll>> finished;
        for (auto config : chain) {
            if (! config || ! started) continue;
            if (config->m_controller != this) continue;
            config->m_controller = nullptr;
            ScrollParticipant(config->item()).end(cancelled);
            if (! guard) return;
            finished.append(config);
        }
        m_wheel    = false;
        m_applying = false;
        for (auto behavior : behaviors) {
            if (behavior) behavior->end(cancelled);
            if (! guard) return;
        }
        m_stopping = false;
        for (auto config : finished) {
            if (config) emit config->scrollFinished(cancelled);
            if (! guard) return;
        }
    }

    void cancelInput() { stop(); }
    void cancelMotion() {
        if (m_motionX.active() || m_motionY.active()) stop();
    }
    Flickable::ScrollConsumption submit(QPointF delta, Flickable::ScrollInput input,
                                        Qt::ScrollPhase phase) {
        Flickable::ScrollConsumption result { {}, delta };
        if (delta.isNull()) {
            if (phase == Qt::ScrollEnd)
                stop(false);
            else if (phase == Qt::ScrollBegin)
                stop();
            return result;
        }
        ScrollParticipant participant(m_config->item());
        if (! participant.available() || ! finite(delta)) return result;
        const bool smooth = input == Flickable::ScrollInput::Smooth && m_config->item()->window();
        QPointF    pending;
        if (smooth && m_wheel && m_smoothWheel) {
            if (m_motionX.active()) pending.setX(m_motionX.target() - m_motionPosition.x());
            if (m_motionY.active()) pending.setY(m_motionY.target() - m_motionPosition.y());
            pending = vectorFromScene(m_config->item(), pending);
        }
        const auto before = participant.position() + pending;
        auto       used   = participant.bounded(before + delta) - before;
        const auto axes   = participant.axes();
        if (! axes.testFlag(Qt::Horizontal)) used.setX(0);
        if (! axes.testFlag(Qt::Vertical)) used.setY(0);
        result.consumed = used;
        result.remaining -= used;
        QPointer<ScrollInputHandler> guard(this);
        QPointer<Flickable>          view(qobject_cast<Flickable*>(m_config->item()));
        scrollWheel(vectorToScene(m_config->item(), used), smooth, phase, true);
        if (guard && view && smooth && ! used.isNull() && m_started) {
            Qt::Orientations moving;
            if (used.x()) moving |= Qt::Horizontal;
            if (used.y()) moving |= Qt::Vertical;
            m_applying = true;
            view->updateScrollActivity(Flickable::ScrollSource::Wheel, moving);
            if (guard) m_applying = false;
        }
        return result;
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide) {
            stop();
        } else if (event->type() == QEvent::MouseButtonPress ||
                   event->type() == QEvent::TouchBegin) {
            if (! m_pressed) stop();
        } else if (event->type() == QEvent::Wheel && m_wheel) {
            auto* wheel = static_cast<QWheelEvent*>(event);
            // Only finish state here. Updates still go through Quick hit testing
            // and popup filters, even when a previous wheel stream is active.
            if (wheel->phase() == Qt::ScrollBegin || wheel->phase() == Qt::ScrollEnd)
                stop(wheel->phase() != Qt::ScrollEnd);
        }
        return false;
    }
    bool wantsPointerEvent(QPointerEvent* event) override {
        if (! m_config || (! m_config->enabled() && ! qobject_cast<Flickable*>(m_config->item())))
            return false;
        if (auto* wheel = dynamic_cast<QWheelEvent*>(event);
            wheel && wheel->phase() == Qt::ScrollEnd)
            return false;
        if (event->type() == QEvent::TouchCancel || event->pointCount() != 1) {
            stop();
            return false;
        }
        if (event->type() != QEvent::Wheel && event->type() != QEvent::TouchBegin &&
            event->type() != QEvent::TouchUpdate && event->type() != QEvent::TouchEnd &&
            event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseButtonRelease && event->type() != QEvent::MouseMove)
            return false;
        if (auto* mouse = dynamic_cast<QMouseEvent*>(event)) {
            auto*      item    = m_config->item();
            const auto buttons = qobject_cast<Flickable*>(item)
                                     ? qobject_cast<Flickable*>(item)->acceptedButtons()
                                     : item->acceptedMouseButtons();
            if (! (buttons & (mouse->button() | mouse->buttons()))) return false;
        }
        if (m_pressed)
            return event->pointingDevice() == m_device && event->point(0).id() == m_pointId;
        if (event->type() != QEvent::Wheel && event->point(0).state() != QEventPoint::Pressed)
            return false;
        for (auto grabber : event->passiveGrabbers(event->point(0))) {
            if (grabber && grabber != this && dynamic_cast<ScrollInputHandler*>(grabber.data()))
                return false;
        }
        if (! ScrollParticipant(m_config->item()).accepts(event->point(0).scenePosition()))
            return false;
        return ! chainFor(m_config->item()).isEmpty();
    }

    void handlePointerEventImpl(QPointerEvent* event) override {
        QPointer<ScrollInputHandler> guard(this);
        if (auto* wheel = dynamic_cast<QWheelEvent*>(event)) {
            handleWheel(wheel);
            return;
        }
        auto&      point = event->point(0);
        const auto scene = point.scenePosition();
        if (point.state() == QEventPoint::Pressed) {
            stop();
            if (! guard) return;
            const auto candidates = chainFor(m_config->item());
            for (auto config : candidates) {
                if (! config) return;
                if (auto* view = qobject_cast<Flickable*>(config->item()))
                    view->cancelScrollActivity();
                if (! guard) return;
            }
            watch(chainFor(m_config->item()));
            if (m_chain.isEmpty()) return;
            m_pressed = true;
            m_device  = event->pointingDevice();
            m_pointId = point.id();
            m_last = m_press = scene;
            m_lastTime       = event->timestamp();
            m_velocity       = {};
            m_gesture.clear();
            m_gesture.sample(scene, event->timestamp());
            setPassiveGrab(event, point);
            if (event->type() == QEvent::MouseButtonPress) point.setAccepted();
            return;
        }
        if (! m_pressed) return;
        if (point.state() == QEventPoint::Updated) {
            m_gesture.sample(scene, event->timestamp());
            bool starting = ! m_dragging;
            if (! m_dragging) {
                const auto total = scene - m_press;
                if (! m_gesture.basis() && ! selectDragAxis(-total)) return;
                if (! guard || ! m_gesture.basis()) return;
                auto* grabber = event->exclusiveGrabber(point);
                if (auto* item = qobject_cast<QQuickItem*>(grabber);
                    item && (item->keepMouseGrab() || item->keepTouchGrab()) &&
                    ! isParticipant(item)) {
                    stop();
                    return;
                }
                if (! setExclusiveGrab(event, point)) {
                    if (guard) stop();
                    return;
                }
                if (! guard) return;
                if (! begin()) return;
                m_dragging = true;
                keepGrab();
                setActive(true);
                if (! guard || ! m_dragging) return;
            }
            auto delta = dragDelta(m_last - scene);
            if (starting && ! ScrollParticipant(m_gesture.basis()).synchronousDrag()) {
                const auto length = std::hypot(delta.x(), delta.y());
                if (length > 0) delta *= std::max(qreal(0), length - dragThreshold()) / length;
            }
            m_velocity = dragDelta(m_gesture.sampleVelocity());
            m_last     = scene;
            m_lastTime = event->timestamp();
            consume(delta, ScrollParticipant::Activity::Drag, m_velocity);
            point.setAccepted();
        } else if (point.state() == QEventPoint::Released) {
            if (! m_dragging) {
                stop();
                return;
            }
            m_pressed  = false;
            m_dragging = false;
            setExclusiveGrab(event, point, false);
            if (! guard) return;
            setPassiveGrab(event, point, false);
            if (! guard) return;
            restoreGrab();
            m_device = nullptr;
            if (event->timestamp() - m_lastTime >= 100) m_velocity = {};
            const ScrollParticipant owner(m_gesture.basis());
            auto       releaseVelocity = vectorFromScene(m_gesture.basis(), m_velocity);
            const auto limit           = owner.maximumVelocity();
            releaseVelocity.setX(std::clamp(releaseVelocity.x(), -limit, limit));
            releaseVelocity.setY(std::clamp(releaseVelocity.y(), -limit, limit));
            m_velocity = vectorToScene(m_gesture.basis(), releaseVelocity);
            if (! releaseConnections()) return;
            const auto chain = m_chain;
            for (auto config : chain) {
                if (config)
                    ScrollParticipant(config->item())
                        .release(vectorFromScene(config->item(), m_velocity));
                if (! guard || m_chain.isEmpty()) return;
            }
            const ScrollParticipant first(m_gesture.basis());
            const auto              maximum       = first.maximumVelocity();
            auto                    localVelocity = vectorFromScene(m_gesture.basis(), m_velocity);
            localVelocity.setX(std::clamp(localVelocity.x(), -maximum, maximum));
            localVelocity.setY(std::clamp(localVelocity.y(), -maximum, maximum));
            if (std::abs(localVelocity.x()) < 75) localVelocity.setX(0);
            if (std::abs(localVelocity.y()) < 75) localVelocity.setY(0);
            m_motionPosition = {};
            if (localVelocity.x())
                m_motionX.fling(0, localVelocity.x(), first.deceleration(), now());
            if (localVelocity.y())
                m_motionY.fling(0, localVelocity.y(), first.deceleration(), now());
            requestFrame();
            if (! m_motionX.active() && ! m_motionY.active()) stop(false);
            point.setAccepted();
        }
    }
    void onGrabChanged(QQuickPointerHandler* grabber, QPointingDevice::GrabTransition transition,
                       QPointerEvent* event, QEventPoint& point) override {
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        if (grabber == this && (transition == QPointingDevice::CancelGrabExclusive ||
                                transition == QPointingDevice::CancelGrabPassive))
            stop();
    }

private:
    QPointF dragDelta(QPointF delta) const { return m_gesture.project(delta); }
    bool    selectDragAxis(QPointF delta) {
        QList<ScrollGesture::Candidate> candidates;
        for (auto config : m_chain) {
            if (! config || ! config->item()) return false;
            auto axes = ScrollParticipant(config->item()).axes() & axesFor(config);
            if (auto* connection = connectionFor(config))
                axes |= connection->dragAxes() & axesFor(config);
            if (axes) candidates.append({ config->item(), axes });
        }
        return m_gesture.select(candidates, delta, dragThreshold());
    }
    void keepGrab() {
        auto* item = parentItem();
        if (! item) return;
        m_grabItem         = item;
        m_hadKeepMouseGrab = item->keepMouseGrab();
        m_hadKeepTouchGrab = item->keepTouchGrab();
        // Native Flickable filters the handler's parent item before delivering updates.
        item->setKeepMouseGrab(true);
        item->setKeepTouchGrab(true);
    }
    void restoreGrab() {
        if (auto item = std::exchange(m_grabItem, {})) {
            item->setKeepMouseGrab(m_hadKeepMouseGrab);
            item->setKeepTouchGrab(m_hadKeepTouchGrab);
        }
    }
    bool releaseConnections() {
        QPointer<ScrollInputHandler> guard(this);
        const auto                   chain    = m_chain;
        const auto                   revision = m_revision;
        for (auto it = chain.crbegin(); it != chain.crend(); ++it) {
            const auto config = *it;
            if (! config || ! config->item()) {
                stop();
                return false;
            }
            if (! connectionFor(config)) continue;
            auto* item      = config->item();
            auto  available = vectorFromScene(item, m_velocity);
            if (! axesFor(config).testFlag(Qt::Horizontal)) available.setX(0);
            if (! axesFor(config).testFlag(Qt::Vertical)) available.setY(0);
            const auto basisX = vectorToScene(item, { 1, 0 });
            const auto basisY = vectorToScene(item, { 0, 1 });
            if (! finite(available) || ! finite(basisX) || ! finite(basisY) ||
                std::abs(basisX.x() * basisY.y() - basisX.y() * basisY.x()) < 1e-9) {
                stop();
                return false;
            }
            m_applying = true;
            const auto used =
                boundedConsumption(connectionFor(config)->release(available), available);
            if (! guard || m_revision != revision || m_chain.isEmpty()) return false;
            m_velocity -= basisX * used.x() + basisY * used.y();
            m_applying = false;
        }
        return true;
    }
    bool isParticipant(QQuickItem* item) const {
        for (auto config : m_chain)
            if (config && config->item() == item) return true;
        return false;
    }
    qreal now() const { return m_clock.nsecsElapsed() / 1e9; }
    void  watch(QList<QPointer<NestedScroll>> chain) {
        ++m_revision;
        m_chain = chain;
        for (auto config : chain) {
            auto* item = config->item();
            m_connections << connect(item, &QQuickItem::windowChanged, this, [this] {
                stop();
            });
            if (auto* owned = qobject_cast<Flickable*>(item))
                m_connections << connect(owned, &Flickable::interactiveChanged, this, [this] {
                    stop();
                });
            if (auto* qt = qobject_cast<QQuickFlickable*>(item))
                m_connections << connect(qt, &QQuickFlickable::interactiveChanged, this, [this] {
                    stop();
                });
            auto interrupt = [this] {
                if (m_started && ! m_applying) stop();
            };
            if (auto* owned = qobject_cast<Flickable*>(item)) {
                m_connections << connect(
                    owned, &Flickable::scrollGeometryAdjusted, this, [this, config](QPointF delta) {
                        if (m_started) m_origins[config] += delta;
                    });
                m_connections << connect(owned, &Flickable::scrollInterrupted, this, [this] {
                    stop();
                });
            }
            if (auto* qt = qobject_cast<QQuickFlickable*>(item)) {
                m_connections << connect(qt, &QQuickFlickable::contentXChanged, this, interrupt);
                m_connections << connect(qt, &QQuickFlickable::contentYChanged, this, interrupt);
            }
            m_connections << connect(config, &QObject::destroyed, this, [this] {
                stop();
            });
            m_connections << connect(config, &NestedScroll::enabledChanged, this, [this] {
                stop();
            });
            m_connections << connect(config, &NestedScroll::axesChanged, this, [this] {
                stop();
            });
            m_connections << connect(config, &NestedScroll::wheelEnabledChanged, this, [this] {
                stop();
            });
            m_connections << connect(config, &NestedScroll::restoreOnReverseChanged, this, [this] {
                stop();
            });
            m_connections << connect(config, &NestedScroll::connectionChanged, this, [this] {
                stop();
            });
            if (auto* connection = connectionFor(config)) {
                m_connections << connect(
                    connection, &NestedScrollConnection::invalidated, this, [this] {
                        stop();
                    });
                m_connections << connect(connection, &QObject::destroyed, this, [this] {
                    stop();
                });
            }
            for (auto* p = item; p; p = p->parentItem()) {
                m_connections << connect(p, &QQuickItem::parentChanged, this, [this] {
                    stop();
                });
                m_connections << connect(p, &QQuickItem::visibleChanged, this, [this] {
                    stop();
                });
                m_connections << connect(p, &QQuickItem::enabledChanged, this, [this] {
                    stop();
                });
            }
        }
        if (auto* window = m_config->item()->window()) {
            m_window = window;
            window->installEventFilter(this);
            m_connections << connect(window, &QQuickWindow::afterAnimating, this, [this] {
                if (m_motionX.active() || m_motionY.active()) requestFrame();
            });
            m_connections << connect(window, &QWindow::activeChanged, this, [this, window] {
                if (! window->isActive()) stop();
            });
        }
    }
    bool begin() {
        QPointer<ScrollInputHandler> guard(this);
        m_started        = true;
        m_applying       = true;
        const auto chain = m_chain;
        m_origins.clear();
        bool connectionAllowed = true;
        for (auto config : chain) {
            if (! config) {
                stop();
                return false;
            }
            if (config->m_controller && config->m_controller != this) config->m_controller->stop();
            if (! guard || ! config || m_chain.isEmpty()) return false;
            config->m_controller = this;
            m_origins.insert(config, ScrollParticipant(config->item()).position());
            ScrollParticipant(config->item())
                .begin(m_wheel ? Flickable::ScrollSource::Wheel : Flickable::ScrollSource::Drag);
            if (! guard || m_chain.isEmpty()) return false;
            if (m_wheel && ! wheelEnabledFor(config)) connectionAllowed = false;
            if (auto* connection = connectionFor(config); connection && connectionAllowed) {
                m_behaviors.append(connection);
                connection->begin(m_wheel ? NestedScrollConnection::Wheel
                                          : NestedScrollConnection::Drag);
                if (! guard || m_chain.isEmpty()) return false;
            }
        }
        m_applying          = false;
        const auto revision = m_revision;
        for (auto config : chain) {
            if (config) emit config->scrollStarted();
            if (! guard || m_revision != revision || m_chain.isEmpty()) return false;
        }
        return true;
    }
    QPointF consume(QPointF delta, ScrollParticipant::Activity activity, QPointF velocity = {}) {
        const QPointer<ScrollInputHandler> guard(this);
        const auto                         revision = m_revision;
        const ScrollDispatchContext        context { m_chain,
                                                     m_origins,
                                                     [this, guard, revision] {
                                                  return guard && m_revision == revision &&
                                                         ! m_chain.isEmpty();
                                              },
                                              [this](bool applying) {
                                                  m_applying = applying;
                                              },
                                              [this] {
                                                  stop();
                                              } };
        return dispatchNestedScroll(context, delta, activity, velocity);
    }
    void handleWheel(QWheelEvent* event) {
        // Quick must not deliver to the old item after a callback destroys it.
        event->point(0).setAccepted();
        event->accept();
        QPointer<ScrollInputHandler> guard(this);
        QPointF                      delta  = -QPointF(event->pixelDelta());
        const bool                   smooth = delta.isNull() && event->phase() == Qt::NoScrollPhase;
        if (delta.isNull())
            delta = -QPointF(event->angleDelta()) / 120 *
                    (24.0 * qGuiApp->styleHints()->wheelScrollLines());
        if (delta.isNull()) return;
        delta = vectorToScene(m_config->item(), delta);
        QPointer<ScrollInputHandler> receiver(this);
        // Orthogonal child lists must not own their parent's wheel animation.
        for (auto config : chainFor(m_config->item(), true)) {
            if (! config || ! config->item()) return;
            if (! wheelEnabledFor(config)) break;
            auto local = vectorFromScene(config->item(), delta);
            if (! axesFor(config).testFlag(Qt::Horizontal)) local.setX(0);
            if (! axesFor(config).testFlag(Qt::Vertical)) local.setY(0);
            auto       content = local;
            const auto axes    = ScrollParticipant(config->item()).axes();
            if (! axes.testFlag(Qt::Horizontal)) content.setX(0);
            if (! axes.testFlag(Qt::Vertical)) content.setY(0);
            const bool accepts =
                ! content.isNull() ||
                (! local.isNull() && connectionFor(config) &&
                 connectionFor(config)->canConsume(local, NestedScrollConnection::Wheel));
            if (! guard || ! config) return;
            if (accepts) {
                receiver = config->m_handler;
                break;
            }
        }
        if (receiver) receiver->scrollWheel(delta, smooth, event->phase());
    }
    void scrollWheel(QPointF delta, bool smooth, Qt::ScrollPhase phase, bool local = false) {
        QPointer<ScrollInputHandler> guard(this);
        if (m_wheel && m_smoothWheel != smooth) stop(false);
        if (! guard) return;
        if (! m_wheel) {
            stop();
            if (! guard) return;
            watch(local ? QList<QPointer<NestedScroll>> { m_config }
                        : chainFor(m_config->item(), true));
            m_wheel       = true;
            m_smoothWheel = smooth;
            if (! begin()) return;
            m_motionPosition = {};
        }
        if (smooth) {
            const qreal time = now();
            if (delta.x())
                m_motionX.wheel(m_motionPosition.x(),
                                (m_motionX.active() ? m_motionX.target() : m_motionPosition.x()) +
                                    delta.x(),
                                time);
            if (delta.y())
                m_motionY.wheel(m_motionPosition.y(),
                                (m_motionY.active() ? m_motionY.target() : m_motionPosition.y()) +
                                    delta.y(),
                                time);
            requestFrame();
        } else {
            QPointer<ScrollInputHandler> guard(this);
            consume(delta, ScrollParticipant::Activity::Scroll);
            if (! guard) return;
            if (phase == Qt::ScrollEnd || phase == Qt::NoScrollPhase) stop(false);
        }
    }
    void requestFrame() {
        if (m_frame && m_config && m_config->item() && m_config->item()->window()) {
            m_frame->polish();
            m_config->item()->window()->update();
        }
    }
    void advance() {
        if (! m_motionX.active() && ! m_motionY.active()) return;
        const auto time = now();
        const auto x = m_motionX.active() ? m_motionX.sample(time)
                                          : ScrollMotion::Sample { m_motionPosition.x(), 0, true };
        const auto y = m_motionY.active() ? m_motionY.sample(time)
                                          : ScrollMotion::Sample { m_motionPosition.y(), 0, true };
        const QPointF next(x.position, y.position);
        auto          delta = next - m_motionPosition;
        if (! m_wheel) delta = vectorToScene(m_gesture.basis(), delta);
        m_motionPosition = next;
        QPointer<ScrollInputHandler> guard(this);
        const auto                   used = consume(
            delta,
            m_wheel ? ScrollParticipant::Activity::Scroll : ScrollParticipant::Activity::Fling,
            m_wheel ? QPointF(x.velocity, y.velocity)
                    : vectorToScene(m_gesture.basis(), { x.velocity, y.velocity }));
        if (! guard || m_chain.isEmpty()) return;
        const auto localDelta = m_wheel ? delta : vectorFromScene(m_gesture.basis(), delta);
        const auto localUsed  = m_wheel ? used : vectorFromScene(m_gesture.basis(), used);
        if (x.finished || (std::abs(localDelta.x()) > 0.001 && std::abs(localUsed.x()) < 0.001))
            m_motionX.stop();
        if (y.finished || (std::abs(localDelta.y()) > 0.001 && std::abs(localUsed.y()) < 0.001))
            m_motionY.stop();
        if (! m_motionX.active() && ! m_motionY.active()) stop(false);
    }
    QPointer<NestedScroll>                  m_config;
    QList<QPointer<NestedScroll>>           m_chain;
    QList<QPointer<NestedScrollConnection>> m_behaviors;
    QHash<NestedScroll*, QPointF>           m_origins;
    QList<QMetaObject::Connection>          m_connections;
    QPointer<const QPointingDevice>         m_device;
    QPointer<QQuickWindow>                  m_window;
    QPointer<ScrollFrameItem>               m_frame;
    QPointer<QQuickItem>                    m_grabItem;
    ScrollGesture                           m_gesture;
    bool                                    m_hadKeepMouseGrab = false, m_hadKeepTouchGrab = false;
    int                                     m_pointId = -1;
    bool          m_pressed = false, m_dragging = false, m_wheel = false, m_smoothWheel = false,
                  m_stopping = false, m_started = false;
    bool          m_applying = false;
    quint64       m_revision = 0;
    QPointF       m_press, m_last, m_velocity, m_motionPosition;
    ulong         m_lastTime = 0;
    QElapsedTimer m_clock;
    ScrollMotion  m_motionX, m_motionY;
};

void ScrollInputAdapter::attach(Flickable* view) {
    auto* config = qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(view));
    ensure(config);
}
void ScrollInputAdapter::ensure(NestedScroll* config) {
    if (config && ! config->m_handler) config->m_handler = new ScrollInputHandler(config);
}
void ScrollInputAdapter::stop(NestedScroll* config) {
    if (config->m_controller) config->m_controller->stop();
    if (config->m_handler) config->m_handler->stop();
}
void ScrollInputAdapter::destroy(NestedScroll* config) {
    stop(config);
    delete config->m_handler;
}
void ScrollInputAdapter::cancel(Flickable* view) {
    auto* config = attached(view);
    if (! config) return;
    const auto handler = config->m_controller ? config->m_controller : config->m_handler;
    if (handler) handler->cancelInput();
}
void ScrollInputAdapter::cancelMotion(Flickable* view) {
    auto* config = attached(view);
    if (! config) return;
    const auto handler = config->m_controller ? config->m_controller : config->m_handler;
    if (handler) handler->cancelMotion();
}
Flickable::ScrollConsumption ScrollInputAdapter::scroll(Flickable* view, QPointF delta,
                                                        Flickable::ScrollInput input,
                                                        Qt::ScrollPhase        phase) {
    attach(view);
    return attached(view)->m_handler->submit(delta, input, phase);
}
bool ScrollInputAdapter::coordinates(QQuickItem* receiver, QQuickItem* ancestor) {
    for (auto config : chainFor(receiver))
        if (config && config->item() == ancestor) return true;
    return false;
}
} // namespace qml_material
