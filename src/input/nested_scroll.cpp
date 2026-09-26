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
QList<QPointer<NestedScroll>> chainFor(QQuickItem* item) {
    QList<QPointer<NestedScroll>> result;
    for (auto* p = item; p; p = p->parentItem()) {
        if (! ScrollParticipant(p).item()) continue;
        auto* config = attached(p);
        if (! config || ! config->enabled() || ! ScrollParticipant(p).available()) break;
        result.append(config);
    }
    return result;
}
QPointF vectorToScene(QQuickItem* item, QPointF delta) {
    return item->mapToScene(delta) - item->mapToScene(QPointF());
}
QPointF vectorFromScene(QQuickItem* item, QPointF delta) {
    return item->mapFromScene(delta) - item->mapFromScene(QPointF());
}
bool finite(QPointF p) { return std::isfinite(p.x()) && std::isfinite(p.y()); }

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

class NestedScrollHandler : public QQuickPointerHandler {
public:
    explicit NestedScrollHandler(NestedScroll* config)
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
    ~NestedScrollHandler() override {
        stop();
        delete m_frame;
    }

    void stop() {
        if (m_stopping) return;
        ++m_revision;
        m_stopping = true;
        QPointer<NestedScrollHandler> guard(this);
        const bool                    started = std::exchange(m_started, false);
        m_pressed                             = false;
        m_dragging                            = false;
        m_motionX.stop();
        m_motionY.stop();
        const auto chain = std::exchange(m_chain, {});
        for (const auto& c : std::exchange(m_connections, {})) disconnect(c);
        if (m_window) m_window->removeEventFilter(this);
        m_window = nullptr;
        if (m_device)
            QPointingDevicePrivate::get(const_cast<QPointingDevice*>(m_device.data()))
                ->removeGrabber(this, true);
        if (! guard) return;
        m_device = nullptr;
        for (auto config : chain) {
            if (! config || ! started) continue;
            if (config->m_controller != this) continue;
            config->m_controller = nullptr;
            ScrollParticipant(config->item()).end();
            if (! guard) return;
        }
        m_wheel    = false;
        m_stopping = false;
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
            if (wheel->phase() == Qt::ScrollBegin || wheel->phase() == Qt::ScrollEnd) stop();
        }
        return false;
    }
    bool wantsPointerEvent(QPointerEvent* event) override {
        if (! m_config || ! m_config->enabled()) return false;
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
        if (auto* mouse = dynamic_cast<QMouseEvent*>(event);
            mouse && mouse->button() != Qt::LeftButton && mouse->buttons() != Qt::LeftButton &&
            mouse->type() != QEvent::MouseMove)
            return false;
        if (m_pressed)
            return event->pointingDevice() == m_device && event->point(0).id() == m_pointId;
        if (event->type() != QEvent::Wheel && event->point(0).state() != QEventPoint::Pressed)
            return false;
        for (auto grabber : event->passiveGrabbers(event->point(0))) {
            if (grabber && grabber != this && dynamic_cast<NestedScrollHandler*>(grabber.data()))
                return false;
        }
        if (! ScrollParticipant(m_config->item()).accepts(event->point(0).scenePosition()))
            return false;
        return chainFor(m_config->item()).size() > 1;
    }

    void handlePointerEventImpl(QPointerEvent* event) override {
        QPointer<NestedScrollHandler> guard(this);
        if (auto* wheel = dynamic_cast<QWheelEvent*>(event)) {
            handleWheel(wheel);
            return;
        }
        auto&      point = event->point(0);
        const auto scene = point.scenePosition();
        if (point.state() == QEventPoint::Pressed) {
            stop();
            if (! guard) return;
            watch(chainFor(m_config->item()));
            m_pressed = true;
            m_device  = event->pointingDevice();
            m_pointId = point.id();
            m_last = m_press = scene;
            m_lastTime       = event->timestamp();
            m_velocity       = {};
            setPassiveGrab(event, point);
            return;
        }
        if (! m_pressed) return;
        if (point.state() == QEventPoint::Updated) {
            if (! m_dragging) {
                const auto total = scene - m_press;
                if (std::hypot(total.x(), total.y()) < dragThreshold()) return;
                if (! canConsume(-total)) return;
                auto* grabber = event->exclusiveGrabber(point);
                if (auto* item = qobject_cast<QQuickItem*>(grabber);
                    item && (item->keepMouseGrab() || item->keepTouchGrab()) &&
                    ! isParticipant(item)) {
                    stop();
                    return;
                }
                if (! setExclusiveGrab(event, point)) {
                    stop();
                    return;
                }
                if (! begin()) return;
                m_dragging = true;
            }
            const auto delta   = m_last - scene;
            const auto elapsed = event->timestamp() - m_lastTime;
            if (elapsed > 0 && elapsed < 150) m_velocity = delta * (1000.0 / elapsed);
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
            setPassiveGrab(event, point, false);
            m_device         = nullptr;
            const auto chain = m_chain;
            for (auto config : chain) {
                if (config) ScrollParticipant(config->item()).end();
                if (! guard || m_chain.isEmpty()) return;
            }
            if (event->timestamp() - m_lastTime > 100) m_velocity = {};
            const ScrollParticipant first(m_config->item());
            const auto              maximum = first.maximumVelocity();
            m_velocity.setX(std::clamp(m_velocity.x(), -maximum, maximum));
            m_velocity.setY(std::clamp(m_velocity.y(), -maximum, maximum));
            if (std::abs(m_velocity.x()) < 75) m_velocity.setX(0);
            if (std::abs(m_velocity.y()) < 75) m_velocity.setY(0);
            m_motionPosition = {};
            if (m_velocity.x()) m_motionX.fling(0, m_velocity.x(), first.deceleration(), now());
            if (m_velocity.y()) m_motionY.fling(0, m_velocity.y(), first.deceleration(), now());
            requestFrame();
            if (! m_motionX.active() && ! m_motionY.active()) stop();
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
                m_connections << connect(owned, &Flickable::contentXChanged, this, interrupt);
                m_connections << connect(owned, &Flickable::contentYChanged, this, interrupt);
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
            m_connections << connect(window, &QWindow::activeChanged, this, [this] {
                stop();
            });
        }
    }
    bool begin() {
        QPointer<NestedScrollHandler> guard(this);
        m_started        = true;
        m_applying       = true;
        const auto chain = m_chain;
        m_origins.clear();
        for (auto config : chain) {
            if (! config) {
                stop();
                return false;
            }
            if (config->m_controller && config->m_controller != this) config->m_controller->stop();
            if (! guard || ! config || m_chain.isEmpty()) return false;
            config->m_controller = this;
            m_origins.insert(config, ScrollParticipant(config->item()).position());
            ScrollParticipant(config->item()).begin();
            if (! guard || m_chain.isEmpty()) return false;
        }
        m_applying = false;
        return true;
    }
    bool canConsume(QPointF sceneDelta) const {
        for (auto config : m_chain) {
            if (! config || ! config->item()) return false;
            ScrollParticipant participant(config->item());
            auto              local = vectorFromScene(config->item(), sceneDelta);
            const auto        axes  = config->axes() & participant.axes();
            if (! axes.testFlag(Qt::Horizontal)) local.setX(0);
            if (! axes.testFlag(Qt::Vertical)) local.setY(0);
            if (finite(local) &&
                participant.bounded(participant.position() + local) != participant.position())
                return true;
        }
        return false;
    }
    QPointF consume(QPointF sceneDelta, ScrollParticipant::Activity activity,
                    QPointF velocity = {}) {
        QPointF                       remaining = sceneDelta;
        QPointer<NestedScrollHandler> guard(this);
        const auto                    chain    = m_chain;
        const auto                    revision = m_revision;
        auto                          order    = chain;
        if (activity == ScrollParticipant::Activity::Drag) {
            for (auto config : chain)
                if (config && config->m_restoreOnReverse) order.prepend(config);
        }
        const auto restoreCount = order.size() - chain.size();
        for (qsizetype index = 0; index < order.size(); ++index) {
            auto config = order[index];
            if (! config || ! config->item()) {
                stop();
                return sceneDelta - remaining;
            }
            QPointer<QQuickItem> item = config->item();
            ScrollParticipant    participant(item);
            if (! participant.available()) {
                stop();
                return sceneDelta - remaining;
            }
            auto local = vectorFromScene(item, remaining);
            if (activity == ScrollParticipant::Activity::Scroll && ! config->m_wheelEnabled) break;
            if (index < restoreCount) {
                const auto back    = m_origins.value(config) - participant.position();
                const auto restore = [](qreal delta, qreal back) {
                    return delta * back > 0
                               ? std::copysign(std::min(std::abs(delta), std::abs(back)), delta)
                               : 0;
                };
                local = { restore(local.x(), back.x()), restore(local.y(), back.y()) };
            }
            if (! config->axes().testFlag(Qt::Horizontal)) local.setX(0);
            if (! config->axes().testFlag(Qt::Vertical)) local.setY(0);
            const auto basisX = vectorToScene(item, { 1, 0 });
            const auto basisY = vectorToScene(item, { 0, 1 });
            if (! finite(local) ||
                std::abs(basisX.x() * basisY.y() - basisX.y() * basisY.x()) < 1e-9) {
                qWarning("NestedScroll cannot consume a non-invertible item transform");
                stop();
                return sceneDelta - remaining;
            }
            m_applying      = true;
            const auto used = participant.consume(
                local, activity, vectorFromScene(item, velocity), [&, revision] {
                    return guard && m_revision == revision && ! m_chain.isEmpty();
                });
            remaining -= basisX * used.x() + basisY * used.y();
            if (! guard || m_revision != revision || m_chain.isEmpty())
                return sceneDelta - remaining;
            m_applying = false;
        }
        return sceneDelta - remaining;
    }
    void handleWheel(QWheelEvent* event) {
        QPointer<NestedScrollHandler> guard(this);
        if (! m_wheel) {
            stop();
            if (! guard) return;
            watch(chainFor(m_config->item()));
            m_wheel = true;
            if (! begin()) return;
            m_motionPosition = {};
        }
        QPointF    delta  = -QPointF(event->pixelDelta());
        const bool smooth = delta.isNull() && event->phase() == Qt::NoScrollPhase;
        if (delta.isNull())
            delta = -QPointF(event->angleDelta()) / 120 *
                    (24.0 * qGuiApp->styleHints()->wheelScrollLines());
        delta = vectorToScene(m_config->item(), delta);
        if (smooth) {
            const auto x = m_motionX.active()
                               ? m_motionX.sample(now())
                               : ScrollMotion::Sample { m_motionPosition.x(), 0, true };
            const auto y = m_motionY.active()
                               ? m_motionY.sample(now())
                               : ScrollMotion::Sample { m_motionPosition.y(), 0, true };
            m_motionX.smooth(m_motionPosition.x(),
                             x.velocity,
                             (m_motionX.active() ? m_motionX.target() : m_motionPosition.x()) +
                                 delta.x(),
                             now());
            m_motionY.smooth(m_motionPosition.y(),
                             y.velocity,
                             (m_motionY.active() ? m_motionY.target() : m_motionPosition.y()) +
                                 delta.y(),
                             now());
            requestFrame();
        } else {
            QPointer<NestedScrollHandler> guard(this);
            consume(delta, ScrollParticipant::Activity::Scroll);
            if (! guard) return;
            if (event->phase() == Qt::ScrollEnd || event->phase() == Qt::NoScrollPhase) stop();
        }
        event->point(0).setAccepted();
        event->accept();
    }
    void requestFrame() {
        if (m_frame && m_config && m_config->item() && m_config->item()->window()) {
            m_frame->polish();
            m_config->item()->window()->update();
        }
    }
    void advance() {
        if (! m_motionX.active() && ! m_motionY.active()) return;
        const auto x = m_motionX.active() ? m_motionX.sample(now())
                                          : ScrollMotion::Sample { m_motionPosition.x(), 0, true };
        const auto y = m_motionY.active() ? m_motionY.sample(now())
                                          : ScrollMotion::Sample { m_motionPosition.y(), 0, true };
        const QPointF next(x.position, y.position);
        const auto    delta = next - m_motionPosition;
        m_motionPosition    = next;
        QPointer<NestedScrollHandler> guard(this);
        const auto                    used = consume(delta,
                                                     m_wheel ? ScrollParticipant::Activity::Scroll
                                                             : ScrollParticipant::Activity::Fling,
                                                     { x.velocity, y.velocity });
        if (! guard || m_chain.isEmpty()) return;
        if (x.finished || (std::abs(delta.x()) > 0.001 && std::abs(used.x()) < 0.001))
            m_motionX.stop();
        if (y.finished || (std::abs(delta.y()) > 0.001 && std::abs(used.y()) < 0.001))
            m_motionY.stop();
        if (! m_motionX.active() && ! m_motionY.active()) stop();
    }
    QPointer<NestedScroll>          m_config;
    QList<QPointer<NestedScroll>>   m_chain;
    QHash<NestedScroll*, QPointF>   m_origins;
    QList<QMetaObject::Connection>  m_connections;
    QPointer<const QPointingDevice> m_device;
    QPointer<QQuickWindow>          m_window;
    QPointer<ScrollFrameItem>       m_frame;
    int                             m_pointId = -1;
    bool          m_pressed = false, m_dragging = false, m_wheel = false, m_stopping = false,
                  m_started  = false;
    bool          m_applying = false;
    quint64       m_revision = 0;
    QPointF       m_press, m_last, m_velocity, m_motionPosition;
    ulong         m_lastTime = 0;
    QElapsedTimer m_clock;
    ScrollMotion  m_motionX, m_motionY;
};

NestedScroll::NestedScroll(QObject* parent)
    : QObject(parent), m_item(qobject_cast<QQuickItem*>(parent)) {}
NestedScroll::~NestedScroll() { delete m_handler; }
NestedScroll* NestedScroll::qmlAttachedProperties(QObject* item) { return new NestedScroll(item); }
void          NestedScroll::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    if (enabled && ! ScrollParticipant(m_item).item()) {
        qWarning("NestedScroll requires a Scrollable or Qt Flickable");
        return;
    }
    m_enabled = enabled;
    if (! m_handler && enabled) m_handler = new NestedScrollHandler(this);
    if (m_handler && ! enabled) m_handler->stop();
    emit enabledChanged();
}
void NestedScroll::setAxes(Qt::Orientations axes) {
    if (m_axes == axes) return;
    m_axes = axes;
    emit axesChanged();
}
bool NestedScroll::coordinates(QQuickItem* receiver, QQuickItem* ancestor) {
    const auto chain = chainFor(receiver);
    if (chain.size() < 2) return false;
    for (auto config : chain)
        if (config && config->item() == ancestor) return true;
    return false;
}
} // namespace qml_material
