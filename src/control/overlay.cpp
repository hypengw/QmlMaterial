#include "qml_material/control/overlay.hpp"
#include "qml_material/control/popup.hpp"
#include "qml_material/control/drawer.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QWheelEvent>
#include <QQmlInfo>
#include <QtQuick/private/qquickwindow_p.h>
#include <algorithm>
#include <utility>

namespace qml_material
{
namespace
{
constexpr auto overlayKey = "_qcm_material_overlay";
class ModalHoverBarrier final : public QQuickItem {
public:
    ModalHoverBarrier(Popup* popup, QQuickItem* parent): QQuickItem(parent), m_popup(popup) {
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::NoButton);
    }
    bool contains(const QPointF& point) const override {
        const auto scenePoint = mapToScene(point);
        return m_popup && m_popup->blocksScenePoint(scenePoint) &&
               m_popup->overlayContainsScenePoint(scenePoint);
    }
private:
    QPointer<Popup> m_popup;
};
}
OverlayManager* OverlayManager::get(QQuickWindow* window) {
    if (! window || QQuickWindowPrivate::get(window)->inDestructor) return nullptr;
    if (auto existing = window->property(overlayKey).value<QObject*>())
        return qobject_cast<OverlayManager*>(existing);
    auto overlay = new OverlayManager(window);
    window->setProperty(overlayKey, QVariant::fromValue<QObject*>(overlay));
    return overlay;
}
bool OverlayManager::isWindowDestroying() const {
    return m_window && QQuickWindowPrivate::get(m_window)->inDestructor;
}
OverlayManager::OverlayManager(QQuickWindow* window)
    : QQuickItem(window->contentItem()), m_window(window) {
    setObjectName(QStringLiteral("Qcm.Material.Overlay"));
    setZ(1000000);
    setAcceptedMouseButtons(Qt::NoButton);
    window->installEventFilter(this);
    connect(window, &QQuickWindow::widthChanged, this, &OverlayManager::resize);
    connect(window, &QQuickWindow::heightChanged, this, &OverlayManager::resize);
    resize();
}
OverlayManager::~OverlayManager() {
    m_destroying = true;
    m_dragDrawer = nullptr;
    if (m_window) {
        m_window->removeEventFilter(this);
        m_window->setProperty(overlayKey, {});
    }
    const auto entries = m_entries;
    for (const auto& entry : entries)
        if (entry.popup) entry.popup->dismissImmediately();
}
void OverlayManager::registerDrawer(Drawer* drawer) {
    if (! m_drawers.contains(drawer)) m_drawers.append(drawer);
}
void OverlayManager::unregisterDrawer(Drawer* drawer) {
    m_drawers.removeAll(drawer);
    releaseDrawer(drawer);
}
Drawer* OverlayManager::dragCandidate(const QPointF& point) const {
    for (auto it = m_entries.crbegin(); it != m_entries.crend(); ++it) {
        auto popup = it->popup;
        if (! popup) continue;
        if (auto drawer = qobject_cast<Drawer*>(popup.data()); drawer && drawer->acceptsDrag(point))
            return drawer;
        if (popup->containsScenePoint(point) || popup->blocksScenePoint(point)) return nullptr;
    }
    Drawer* candidate = nullptr;
    for (const auto& drawer : m_drawers) {
        if (drawer && ! drawer->isVisible() && drawer->acceptsDrag(point) &&
            (! candidate || drawer->z() >= candidate->z()))
            candidate = drawer;
    }
    return candidate;
}
void OverlayManager::cancelDrawerDrag() {
    auto drawer = m_dragDrawer;
    releaseDrawer(drawer);
    if (drawer) drawer->cancelDrag();
}
void OverlayManager::releaseDrawer(Drawer* drawer) {
    if (m_dragDrawer != drawer) return;
    m_dragDrawer = nullptr;
    m_dragDevice = nullptr;
    m_dragId     = -1;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    ungrabMouse();
    ungrabTouchPoints();
}
bool OverlayManager::drawerPointer(QPointerEvent* event, const QEventPoint& point) {
    if (point.state() == QEventPoint::Pressed && ! m_dragDrawer) {
        m_dragDrawer = dragCandidate(point.scenePosition());
        if (m_dragDrawer) {
            m_dragDevice = event->pointingDevice();
            m_dragId     = point.id();
            m_dragDrawer->pressDrag(point.scenePosition(), event->timestamp());
        }
        return false;
    }
    if (! m_dragDrawer || m_dragDevice != event->pointingDevice() || point.id() != m_dragId)
        return false;
    QPointer<Drawer> drawer = m_dragDrawer;
    if (point.state() == QEventPoint::Released) {
        const bool dragging = drawer->dragging();
        m_dragDrawer        = nullptr;
        m_dragDevice        = nullptr;
        m_dragId            = -1;
        if (dragging) {
            event->setExclusiveGrabber(point, nullptr);
            setKeepMouseGrab(false);
            setKeepTouchGrab(false);
            drawer->releaseDrag(point.scenePosition(), event->timestamp());
        }
        return dragging;
    }
    if (point.state() != QEventPoint::Updated) return false;
    if (! drawer->dragging()) {
        QObject* grabber = event->exclusiveGrabber(point);
        if (! grabber && dynamic_cast<QMouseEvent*>(event)) grabber = m_window->mouseGrabberItem();
        if (grabber && grabber != this) {
            auto item = qobject_cast<QQuickItem*>(grabber);
            if (! item || item->keepMouseGrab() || item->keepTouchGrab()) {
                cancelDrawerDrag();
                return false;
            }
        }
        if (! drawer->wantsDrag(point.scenePosition())) return false;
        drawer->startDrag(point.scenePosition());
        if (! drawer || ! drawer->dragging()) return false;
        event->setExclusiveGrabber(point, this);
        if (! drawer || ! drawer->dragging() || m_dragDrawer != drawer) {
            event->setExclusiveGrabber(point, nullptr);
            return true;
        }
        setKeepMouseGrab(true);
        setKeepTouchGrab(true);
        cancelPress();
        m_touchId             = -1;
    } else if (event->exclusiveGrabber(point) != this) {
        cancelDrawerDrag();
        return false;
    }
    if (drawer) drawer->moveDrag(point.scenePosition());
    return true;
}
void OverlayManager::resize() {
    if (! m_window) return;
    setSize(m_window->size());
    const auto entries = m_entries;
    for (const auto& entry : entries) {
        if (entry.hoverBarrier) entry.hoverBarrier->setSize(size());
        if (entry.popup) entry.popup->reposition();
    }
}
void OverlayManager::add(Popup* popup) {
    QPointer<Popup> guard(popup);
    if (m_dragDrawer && m_dragDrawer != popup && popup->modal()) cancelDrawerDrag();
    if (! guard) return;
    for (const auto& entry : std::as_const(m_entries))
        if (entry.popup == popup) return;
    Entry entry;
    entry.popup         = popup;
    entry.previousFocus = m_window ? m_window->activeFocusItem() : nullptr;
    m_entries.append(entry);
    popup->surfaceItem()->setParentItem(this);
    refresh();
}
void OverlayManager::remove(Popup* popup) {
    popup->cancelOverlayPress();
    m_pressObservers.removeAll(popup);
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].popup != popup) continue;
        const auto entry = m_entries.takeAt(i);
        for (auto& remaining : m_entries) {
            if (remaining.previousFocus &&
                (remaining.previousFocus == popup->surfaceItem() ||
                 popup->surfaceItem()->isAncestorOf(remaining.previousFocus)))
                remaining.previousFocus = entry.previousFocus;
        }
        delete entry.dimmer;
        delete entry.hoverBarrier;
        if (! m_destroying) restoreFocus(popup, entry.previousFocus);
        return;
    }
}
void OverlayManager::restoreFocus(Popup* popup, QQuickItem* previous) {
    if (! m_window || isWindowDestroying()) return;
    auto current = m_window->activeFocusItem();
    if (current && current != popup->surfaceItem() && ! popup->surfaceItem()->isAncestorOf(current))
        return;
    if (previous && previous->window() == m_window && previous->isVisible() &&
        previous->isEnabled()) {
        auto modal = topModal();
        if (! modal || previous == modal->surfaceItem() ||
            modal->surfaceItem()->isAncestorOf(previous)) {
            previous->forceActiveFocus(Qt::PopupFocusReason);
            return;
        }
    }
    for (auto it = m_entries.crbegin(); it != m_entries.crend(); ++it)
        if (it->popup && it->popup->focus() && ! it->popup->closing()) {
            it->popup->forceActiveFocus(Qt::PopupFocusReason);
            return;
        }
}
void OverlayManager::refresh() {
    std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
        return a.popup && b.popup && a.popup->z() < b.popup->z();
    });
    // Components may run arbitrary QML while their dimmer is created.
    const auto entries = m_entries;
    qreal      rank    = 0;
    for (const auto& entry : entries) {
        auto popup = entry.popup;
        if (! popup) continue;
        auto                    attached = qmlAttachedPropertiesObject<Overlay>(popup, false);
        auto                    settings = qobject_cast<OverlayAttached*>(attached);
        QPointer<QQmlComponent> component =
            popup->dim() && settings ? (popup->modal() ? settings->modal() : settings->modeless())
                                     : nullptr;
        QPointer<QQuickItem> dimmer  = entry.dimmer;
        qreal                opacity = entry.opacity;
        if (component != entry.component) {
            delete dimmer;
            dimmer = nullptr;
            if (component) {
                auto              context = component->creationContext();
                QPointer<QObject> object =
                    component->beginCreate(context ? context : qmlContext(popup));
                dimmer = qobject_cast<QQuickItem*>(object);
                if (dimmer && popup) {
                    dimmer->setParent(popup);
                    dimmer->setParentItem(this);
                    dimmer->setSize(size());
                }
                if (component) component->completeCreate();
                if (dimmer) {
                    opacity = dimmer->opacity();
                    dimmer->setOpacity(0);
                }
                if (object && ! dimmer) {
                    delete object;
                    qmlWarning(this) << "Overlay dimmer must be an Item";
                }
            }
            auto it = std::find_if(m_entries.begin(), m_entries.end(), [popup](const Entry& value) {
                return value.popup == popup;
            });
            if (! popup || it == m_entries.end()) {
                delete dimmer;
                continue;
            }
            it->dimmer    = dimmer;
            it->component = component;
            it->opacity   = opacity;
        }
        if (dimmer) {
            dimmer->setZ(rank++);
            if (popup) popup->updateDimmer(dimmer, opacity);
        }
        if (popup) {
            auto it = std::find_if(m_entries.begin(), m_entries.end(), [popup](const Entry& value) {
                return value.popup == popup;
            });
            if (it == m_entries.end()) continue;
            if (popup->modal()) {
                if (!it->hoverBarrier) it->hoverBarrier = new ModalHoverBarrier(popup, this);
                it->hoverBarrier->setSize(size());
                it->hoverBarrier->setZ(rank++);
            } else {
                delete it->hoverBarrier;
                it->hoverBarrier = nullptr;
            }
            popup->surfaceItem()->setZ(rank++);
        }
    }
    if (m_window) m_window->update();
}
void OverlayManager::updateDimmer(Popup* popup) {
    for (const auto& entry : std::as_const(m_entries)) {
        if (entry.popup == popup && entry.dimmer) {
            popup->updateDimmer(entry.dimmer, entry.opacity);
            return;
        }
    }
}
Popup* OverlayManager::topModal() const {
    for (auto it = m_entries.crbegin(); it != m_entries.crend(); ++it)
        if (it->popup && it->popup->modal()) return it->popup;
    return nullptr;
}
bool OverlayManager::blocks(const QPointF& point) const {
    for (auto it = m_entries.crbegin(); it != m_entries.crend(); ++it) {
        if (! it->popup) continue;
        if (it->popup->containsScenePoint(point)) return false;
        if (it->popup->blocksScenePoint(point)) return true;
    }
    return false;
}
bool OverlayManager::press(const QPointF& point) {
    cancelPress();
    m_pressed          = true;
    m_blocked          = blocks(point);
    const auto entries = m_entries;
    for (auto it = entries.crbegin(); it != entries.crend(); ++it) {
        auto popup = it->popup;
        if (! popup || ! popup->isVisible() || ! popup->overlayContainsScenePoint(point)) continue;
        const bool pointBlocked  = popup->blocksScenePoint(point);
        const bool outside       = ! popup->containsScenePoint(point);
        m_pressObservers.append(popup);
        popup->overlayPress(point);
        if (!outside || pointBlocked) break;
    }
    return m_blocked;
}
bool OverlayManager::release(const QPointF& point) {
    if (! m_pressed) return blocks(point);
    const auto observers = std::exchange(m_pressObservers, {});
    const bool blocked = m_blocked;
    m_pressed          = false;
    m_blocked          = false;
    for (const auto& popup : observers)
        if (popup) popup->overlayRelease(point);
    return blocked;
}
void OverlayManager::cancelPress() {
    const auto observers = std::exchange(m_pressObservers, {});
    for (const auto& popup : observers)
        if (popup) popup->cancelOverlayPress();
    m_pressed = m_blocked = false;
}
bool OverlayManager::eventFilter(QObject* watched, QEvent* event) {
    if (watched != m_window || m_destroying) return false;
    bool blocked = false;
    if (auto pointer = dynamic_cast<QPointerEvent*>(event);
        pointer &&
        (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove ||
         event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchBegin ||
         event->type() == QEvent::TouchUpdate || event->type() == QEvent::TouchEnd)) {
        if (auto mouse = dynamic_cast<QMouseEvent*>(event);
            mouse &&
            ((event->type() != QEvent::MouseMove && mouse->button() != Qt::LeftButton) ||
             (event->type() == QEvent::MouseMove && ! mouse->buttons().testFlag(Qt::LeftButton)))) {
            // Only the primary button participates in drawer dragging.
        } else {
            for (const auto& point : pointer->points()) blocked |= drawerPointer(pointer, point);
            if (blocked) {
                event->accept();
                return true;
            }
        }
    }
    switch (event->type()) {
    case QEvent::Hide: {
        cancelDrawerDrag();
        cancelPress();
        m_touchId = -1;
        const auto entries = m_entries;
        for (const auto& entry : entries)
            if (entry.popup) entry.popup->dismissImmediately();
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        blocked = press(static_cast<QMouseEvent*>(event)->position());
        break;
    case QEvent::MouseButtonRelease:
        blocked = release(static_cast<QMouseEvent*>(event)->position());
        break;
    case QEvent::MouseMove:
        blocked = m_pressed && m_blocked;
        break;
    case QEvent::Wheel: blocked = blocks(static_cast<QWheelEvent*>(event)->position()); break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        for (const auto& point : static_cast<QTouchEvent*>(event)->points()) {
            if (point.state() == QEventPoint::Pressed && m_touchId < 0) {
                m_touchId = point.id();
                blocked   = press(point.position());
            } else if (point.id() == m_touchId) {
                blocked = m_blocked;
                if (point.state() == QEventPoint::Released) {
                    blocked   = release(point.position());
                    m_touchId = -1;
                }
            }
        }
        break;
    }
    case QEvent::TouchCancel:
        cancelDrawerDrag();
        blocked   = m_blocked;
        cancelPress();
        m_touchId             = -1;
        break;
    case QEvent::KeyPress: {
        auto key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Escape || key->key() == Qt::Key_Back) {
            const auto entries = m_entries;
            for (auto it = entries.crbegin(); it != entries.crend(); ++it) {
                if (! it->popup) continue;
                if (it->popup->closePolicy().testFlag(Popup::CloseOnEscape)) {
                    if (! it->popup->closing()) it->popup->closeFromInput();
                    blocked = true;
                    break;
                }
                if (it->popup->modal()) {
                    blocked = true;
                    break;
                }
            }
        } else if (auto popup = topModal()) {
            auto surface = popup->surfaceItem();
            auto active  = m_window->activeFocusItem();
            if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) {
                const bool forward =
                    key->key() != Qt::Key_Backtab && ! (key->modifiers() & Qt::ShiftModifier);
                auto       next  = active ? active : surface;
                const auto start = next;
                do {
                    next = next->nextItemInFocusChain(forward);
                    if (next != surface && surface->isAncestorOf(next) &&
                        next->activeFocusOnTab() && next->isVisible() && next->isEnabled())
                        break;
                } while (next != start);
                if (next == surface || ! surface->isAncestorOf(next)) next = surface;
                next->forceActiveFocus(forward ? Qt::TabFocusReason : Qt::BacktabFocusReason);
                blocked = true;
            } else
                blocked = ! active || (active != surface && ! surface->isAncestorOf(active));
        }
        break;
    }
    default: break;
    }
    if (blocked) event->accept();
    return blocked;
}

OverlayAttached* Overlay::qmlAttachedProperties(QObject* object) {
    return new OverlayAttached(object);
}
OverlayAttached::OverlayAttached(QObject* object): QObject(object) {
    if (auto popup = qobject_cast<Popup*>(object))
        connect(popup, &Popup::overlayItemChanged, this, &OverlayAttached::overlayChanged);
    else if (auto item = qobject_cast<QQuickItem*>(object))
        connect(item, &QQuickItem::windowChanged, this, &OverlayAttached::overlayChanged);
}
QQuickWindow* OverlayAttached::window() const {
    if (auto popup = qobject_cast<Popup*>(parent()))
        return popup->parentItem() ? popup->parentItem()->window() : nullptr;
    if (auto item = qobject_cast<QQuickItem*>(parent())) return item->window();
    return qobject_cast<QQuickWindow*>(parent());
}
QQuickItem* OverlayAttached::overlay() const { return OverlayManager::get(window()); }
void        OverlayAttached::setModal(QQmlComponent* value) {
    if (m_modal == value) return;
    m_modal = value;
    if (auto manager = OverlayManager::get(window())) manager->refresh();
    Q_EMIT modalChanged();
}
void OverlayAttached::setModeless(QQmlComponent* value) {
    if (m_modeless == value) return;
    m_modeless = value;
    if (auto manager = OverlayManager::get(window())) manager->refresh();
    Q_EMIT modelessChanged();
}
} // namespace qml_material
