#include "qml_material/control/popup.hpp"
#include "popup_surface.hpp"
#include "qml_material/control/overlay.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/util/qt.hpp"
#include <QQuickWindow>
#include <QGuiApplication>
#include <QQmlInfo>
#include <QtQuick/private/qquicktransitionmanager_p_p.h>
#include <algorithm>

namespace qml_material
{
class PopupMotion : public QQuickTransitionManager {
public:
    explicit PopupMotion(Popup* popup): m_popup(popup) {}

protected:
    void finished() override {
        if (! m_popup) return;
        if (m_popup->entering())
            m_popup->completeEnter();
        else if (m_popup->closing())
            m_popup->completeExit();
    }

private:
    QPointer<Popup> m_popup;
};

Popup::Popup(QObject* parent): Popup(new PopupSurface<Panel>(this), parent) {}
Popup::Popup(Panel* surface, QObject* parent)
    : QObject(parent), m_surface(surface), m_motion(std::make_unique<PopupMotion>(this)) {
    m_surface->setParent(this);
    m_surface->setVisible(false);
    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, &Popup::completeExit);
    connect(m_surface, &Panel::contentItemChanged, this, &Popup::contentItemChanged);
    connect(m_surface, &Panel::backgroundChanged, this, &Popup::backgroundChanged);
    connect(m_surface, &Panel::widthChanged, this, &Popup::widthChanged);
    connect(m_surface, &Panel::heightChanged, this, &Popup::heightChanged);
    connect(m_surface, &Panel::implicitWidthChanged, this, &Popup::implicitWidthChanged);
    connect(m_surface, &Panel::implicitHeightChanged, this, &Popup::implicitHeightChanged);
    connect(m_surface, &Panel::contentWidthChanged, this, &Popup::contentWidthChanged);
    connect(m_surface, &Panel::contentHeightChanged, this, &Popup::contentHeightChanged);
    connect(m_surface, &Panel::paddingChanged, this, &Popup::paddingChanged);
    connect(m_surface, &Panel::horizontalPaddingChanged, this, &Popup::horizontalPaddingChanged);
    connect(m_surface, &Panel::verticalPaddingChanged, this, &Popup::verticalPaddingChanged);
    connect(m_surface, &Panel::leftPaddingChanged, this, &Popup::leftPaddingChanged);
    connect(m_surface, &Panel::rightPaddingChanged, this, &Popup::rightPaddingChanged);
    connect(m_surface, &Panel::topPaddingChanged, this, &Popup::topPaddingChanged);
    connect(m_surface, &Panel::bottomPaddingChanged, this, &Popup::bottomPaddingChanged);
    connect(m_surface, &Panel::spacingChanged, this, &Popup::spacingChanged);
    connect(m_surface, &Panel::leftInsetChanged, this, &Popup::leftInsetChanged);
    connect(m_surface, &Panel::rightInsetChanged, this, &Popup::rightInsetChanged);
    connect(m_surface, &Panel::topInsetChanged, this, &Popup::topInsetChanged);
    connect(m_surface, &Panel::bottomInsetChanged, this, &Popup::bottomInsetChanged);
    connect(m_surface, &Panel::opacityChanged, this, &Popup::opacityChanged);
    connect(m_surface, &Panel::scaleChanged, this, &Popup::scaleChanged);
    connect(m_surface, &Panel::transformOriginChanged, this, &Popup::transformOriginChanged);
    connect(m_surface, &Panel::enabledChanged, this, &Popup::enabledChanged);
    connect(m_surface, &Panel::availableWidthChanged, this, &Popup::availableWidthChanged);
    connect(m_surface, &Panel::availableHeightChanged, this, &Popup::availableHeightChanged);
    connect(
        m_surface, &Panel::implicitContentWidthChanged, this, &Popup::implicitContentWidthChanged);
    connect(m_surface,
            &Panel::implicitContentHeightChanged,
            this,
            &Popup::implicitContentHeightChanged);
    connect(m_surface,
            &Panel::implicitBackgroundWidthChanged,
            this,
            &Popup::implicitBackgroundWidthChanged);
    connect(m_surface,
            &Panel::implicitBackgroundHeightChanged,
            this,
            &Popup::implicitBackgroundHeightChanged);
    connect(m_surface, &Panel::activeFocusChanged, this, &Popup::activeFocusChanged);
    connect(m_surface, &Panel::visualFocusChanged, this, &Popup::visualFocusChanged);
    connect(m_surface, &Panel::mirroredChanged, this, &Popup::mirroredChanged);
    connect(m_surface, &Panel::contentChildrenChanged, this, &Popup::contentChildrenChanged);
    connect(m_surface, &Panel::fontChanged, this, &Popup::fontChanged);
    connect(m_surface, &Panel::localeChanged, this, &Popup::localeChanged);
    connect(m_surface, &Panel::widthChanged, this, &Popup::reposition);
    connect(m_surface, &Panel::heightChanged, this, &Popup::reposition);
    connect(qGuiApp, &QGuiApplication::fontChanged, this, &Popup::refreshEnvironment);
    connect(qGuiApp, &QGuiApplication::layoutDirectionChanged, this, &Popup::refreshEnvironment);
}
Popup::~Popup() {
    m_dismissing = true;
    disconnect(m_presentationOwnerConnection);
    m_presentationOwner = nullptr;
    utils::disconnectAll(m_parentConnections);
    utils::disconnectAll(m_positioningConnections);
    utils::disconnectAll(m_overlayConnections);
    m_motion->cancel();
    if (m_overlay) m_overlay->remove(this);
    m_surface->setParentItem(nullptr);
    delete m_surface;
}
void Popup::classBegin() {
    QQmlEngine::setContextForObject(m_surface, qmlContext(this));
    static_cast<QQmlParserStatus*>(m_surface)->classBegin();
    qmlAttachedPropertiesObject<Theme>(this, true);
    auto theme = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(m_surface, true));
    theme->setInheritFrom(this);
}
void Popup::componentComplete() {
    static_cast<QQmlParserStatus*>(m_surface)->componentComplete();
    if (! m_parent) {
        for (auto owner = QObject::parent(); owner; owner = owner->parent()) {
            if (auto item = qobject_cast<QQuickItem*>(owner)) {
                setParentItem(item);
                break;
            }
            if (auto window = qobject_cast<QQuickWindow*>(owner)) {
                setParentItem(window->contentItem());
                break;
            }
        }
    }
    m_complete = true;
    if (m_requestedVisible) open();
}
void Popup::setParentItem(QQuickItem* item) {
    if (m_parent == item) return;
    QPointer<Popup> guard(this);
    if (isVisible()) dismissImmediately();
    if (! guard) return;
    m_parent = item;
    observeParent();
    if (guard) Q_EMIT parentChanged();
}
void Popup::setPositioningItem(QQuickItem* item) {
    if (m_positioningItem == item) return;
    m_positioningItem = item;
    QPointer<Popup> guard(this);
    observePositioningItem();
    if (guard) Q_EMIT positioningItemChanged();
}
void Popup::observePositioningItem() {
    utils::disconnectAll(m_positioningConnections);
    for (auto item = m_positioningItem.data(); item; item = item->parentItem()) {
        for (auto signal : { &QQuickItem::xChanged,
                             &QQuickItem::yChanged,
                             &QQuickItem::widthChanged,
                             &QQuickItem::heightChanged,
                             &QQuickItem::rotationChanged,
                             &QQuickItem::scaleChanged })
            m_positioningConnections.append(connect(item, signal, this, &Popup::reposition));
        m_positioningConnections.append(
            connect(item, &QQuickItem::parentChanged, this, &Popup::observePositioningItem));
        m_positioningConnections.append(
            connect(item, &QQuickItem::transformOriginChanged, this, &Popup::reposition));
    }
    if (m_positioningItem) {
        m_positioningConnections.append(
            connect(m_positioningItem, &QQuickItem::windowChanged, this, &Popup::reposition));
        m_positioningConnections.append(
            connect(m_positioningItem, &QObject::destroyed, this, [this] {
                m_positioningItem = nullptr;
                QPointer<Popup> guard(this);
                observePositioningItem();
                if (guard) Q_EMIT positioningItemChanged();
            }));
    }
    reposition();
}
void Popup::observeParent() {
    utils::disconnectAll(m_parentConnections);
    for (auto item = m_parent.data(); item; item = item->parentItem()) {
        for (auto signal : { &QQuickItem::xChanged,
                             &QQuickItem::yChanged,
                             &QQuickItem::widthChanged,
                             &QQuickItem::heightChanged,
                             &QQuickItem::rotationChanged,
                             &QQuickItem::scaleChanged })
            m_parentConnections.append(connect(item, signal, this, &Popup::reposition));
        m_parentConnections.append(
            connect(item, &QQuickItem::parentChanged, this, &Popup::observeParent));
        m_parentConnections.append(
            connect(item, &QQuickItem::transformOriginChanged, this, &Popup::reposition));
        if (auto control = qobject_cast<Control*>(item)) {
            m_parentConnections.append(
                connect(control, &Control::fontChanged, this, &Popup::refreshEnvironment));
            m_parentConnections.append(
                connect(control, &Control::localeChanged, this, &Popup::refreshEnvironment));
            m_parentConnections.append(connect(
                control, &Control::layoutDirectionChanged, this, &Popup::refreshEnvironment));
            m_parentConnections.append(
                connect(control, &Control::hoverEnabledChanged, this, &Popup::refreshEnvironment));
        }
    }
    if (m_parent) {
        m_parentConnections.append(connect(m_parent, &QQuickItem::windowChanged, this, [this] {
            QPointer<Popup> guard(this);
            updateOverlay();
            if (guard && m_complete && m_requestedVisible && m_overlay && ! isVisible()) open();
        }));
        m_parentConnections.append(connect(m_parent, &QQuickItem::visibleChanged, this, [this] {
            if (m_parent && ! m_parent->isVisible()) dismissImmediately();
        }));
        m_parentConnections.append(connect(m_parent, &QObject::destroyed, this, [this] {
            m_parent = nullptr;
            QPointer<Popup> guard(this);
            dismissImmediately();
            if (! guard) return;
            updateOverlay();
            if (guard) Q_EMIT parentChanged();
        }));
    }
    refreshEnvironment();
    updateOverlay();
    reposition();
}
void Popup::refreshEnvironment() {
    QFont   inherited = QGuiApplication::font();
    QLocale locale;
    auto    direction = QGuiApplication::layoutDirection();
    bool    hover     = QGuiApplication::styleHints()->useHoverEffects();
    for (auto item = m_parent.data(); item; item = item->parentItem()) {
        if (auto control = qobject_cast<Control*>(item)) {
            inherited = control->effectiveFont();
            locale    = control->locale();
            direction = control->layoutDirection();
            hover     = control->hoverEnabled();
            break;
        }
    }
    auto resolved = utils::resolveFont(m_font, inherited);
    // The surface is reparented visually, but its environment belongs to the logical parent.
    resolved.setResolveMask(QFont::AllPropertiesResolved);
    m_surface->setFont(resolved);
    m_surface->setLocale(m_locale.value_or(locale));
    m_surface->setLayoutDirection(direction);
    m_surface->setHoverEnabled(inheritsHoverEnabled() && hover);
}
void Popup::setFont(const QFont& value) {
    m_font = value;
    refreshEnvironment();
}
void Popup::resetFont() {
    m_font = QFont();
    refreshEnvironment();
}
void Popup::setLocale(const QLocale& value) {
    m_locale = value;
    refreshEnvironment();
}
void Popup::resetLocale() {
    m_locale.reset();
    refreshEnvironment();
}
void Popup::updateOverlay() {
    // Let Qt detach the item tree before the overlay closes its popups on destruction.
    // Reparenting a focused surface during window teardown can access a deleted focus scope.
    if (m_overlay && m_overlay->isWindowDestroying()) return;
    auto next = OverlayManager::get(m_parent ? m_parent->window() : nullptr);
    if (m_overlay == next) return;
    QPointer<Popup> guard(this);
    if (isVisible()) dismissImmediately();
    if (! guard) return;
    utils::disconnectAll(m_overlayConnections);
    m_overlay = next;
    if (m_overlay) {
        m_overlayConnections.append(
            connect(m_overlay, &QQuickItem::widthChanged, this, &Popup::overlayGeometryChanged));
        m_overlayConnections.append(
            connect(m_overlay, &QQuickItem::heightChanged, this, &Popup::overlayGeometryChanged));
        m_overlayConnections.append(connect(m_overlay, &QObject::destroyed, this, [this] {
            m_overlay = nullptr;
            Q_EMIT overlayItemChanged();
            Q_EMIT overlayGeometryChanged();
        }));
    }
    Q_EMIT overlayItemChanged();
    if (guard) Q_EMIT overlayGeometryChanged();
}
QQuickItem* Popup::overlayItem() const { return m_overlay; }
qreal       Popup::overlayWidth() const { return m_overlay ? m_overlay->width() : 0; }
qreal       Popup::overlayHeight() const { return m_overlay ? m_overlay->height() : 0; }
void        Popup::setPopupItem(QQuickItem* item) {
    if (m_hitItem == item) return;
    m_hitItem = item;
    Q_EMIT popupItemChanged();
}
void Popup::setVisible(bool value) {
    if (value && ! requestPresentation()) return;
    m_requestedVisible = value;
    if (! m_complete) return;
    if (value)
        open();
    else
        close();
}
bool Popup::changeState(State state) {
    if (m_state == state) return true;
    const auto old = m_state;
    m_state        = state;
    QPointer<Popup> guard(this);
    if ((old == Closed) != (state == Closed)) Q_EMIT visibleChanged();
    if (! guard || m_state != state) return false;
    if ((old == Entering) != (state == Entering)) Q_EMIT enteringChanged();
    if (! guard || m_state != state) return false;
    if ((old == Open) != (state == Open)) Q_EMIT openedChanged();
    if (! guard || m_state != state) return false;
    if ((old == Closing) != (state == Closing)) Q_EMIT closingChanged();
    return guard && m_state == state;
}
bool Popup::acquirePresentation(QObject* owner) {
    if (! owner || (m_presentationOwner && m_presentationOwner != owner)) return false;
    if (m_presentationOwner == owner) return true;
    m_presentationOwner           = owner;
    m_presentationAllowed         = false;
    m_presentationRequestEnabled  = false;
    m_presentationOwnerConnection = connect(owner, &QObject::destroyed, this, [this] {
        QPointer<Popup> guard(this);
        dismissImmediately();
        if (guard) Q_EMIT presentationOwnerChanged();
    });
    QPointer<Popup> guard(this);
    dismissImmediately();
    if (! guard) return false;
    Q_EMIT presentationOwnerChanged();
    return guard && m_presentationOwner == owner;
}
void Popup::releasePresentation(QObject* owner) {
    if (! owner || m_presentationOwner != owner) return;
    QPointer<Popup> guard(this);
    setPresentationAllowed(owner, false);
    if (! guard || m_presentationOwner != owner) return;
    disconnect(m_presentationOwnerConnection);
    m_presentationOwner = nullptr;
    Q_EMIT presentationOwnerChanged();
}
void Popup::setPresentationAllowed(QObject* owner, bool allowed) {
    if (! owner || m_presentationOwner != owner) return;
    m_presentationAllowed = allowed;
    if (! allowed) dismissImmediately();
}
void Popup::setPresentationRequestEnabled(QObject* owner, bool enabled) {
    if (owner && m_presentationOwner == owner) m_presentationRequestEnabled = enabled;
}
bool Popup::requestPresentation() {
    if (m_dismissing) return false;
    if (presentationAllowed()) return true;
    if (! m_presentationRequestEnabled || m_requestingPresentation) return false;
    QPointer<Popup>         guard(this);
    const QPointer<QObject> owner = m_presentationOwner;
    m_requestingPresentation      = true;
    Q_EMIT presentationRequested();
    if (! guard) return false;
    m_requestingPresentation = false;
    return owner && m_presentationOwner == owner && m_presentationAllowed;
}
void Popup::open() {
    if (! requestPresentation()) return;
    if (! m_complete) {
        m_requestedVisible = true;
        return;
    }
    if (entering() || isOpened()) return;
    cancelOverlayPress();
    QPointer<Popup> guard(this);
    updateOverlay();
    if (! guard) return;
    if (! m_overlay) {
        m_requestedVisible = true;
        return;
    }
    m_motion->cancel();
    m_hideTimer.stop();
    m_requestedVisible = true;
    if (! isVisible()) {
        m_savedOpacity = opacity();
        m_savedScale   = scale();
    }
    if (! changeState(Entering)) return;
    m_overlay->add(this);
    if (! guard || ! entering()) return;
    m_surface->setVisible(true);
    reposition();
    refreshEnvironment();
    if (m_focus) forceActiveFocus(Qt::PopupFocusReason);
    if (! guard || ! entering()) return;
    Q_EMIT aboutToShow();
    if (! guard || ! entering() || m_deferred || m_interacting) return;
    startTransition(true);
}
void Popup::beginInteractiveTransition() {
    if (! presentationAllowed()) return;
    m_motion->cancel();
    m_hideTimer.stop();
    m_interacting = true;
    Popup::open();
}
void Popup::endInteractiveTransition(bool opening) {
    m_interacting = false;
    if (! isVisible()) return;
    if (! opening) {
        Popup::close();
        return;
    }
    if (isOpened() && ! changeState(Entering)) return;
    if (entering() && ! m_deferred) startTransition(true);
}
void Popup::startTransition(bool opening) {
    QPointer<Popup> guard(this);
    auto            transition = opening ? m_enter : m_exit;
    if (transition && ! transition->enabled()) transition = nullptr;
    const auto               targets = transitionTargets(opening);
    QList<QQuickStateAction> actions;
    for (const auto& target : targets)
        actions.append(QQuickStateAction(this, target.property, target.value));
    if (transition && targets.size() == 1) {
        qmlExecuteDeferred(transition);
        if (! guard || ! transition || (opening ? ! entering() : ! closing())) return;
        auto animations = transition->animations();
        for (qsizetype i = 0; i < animations.count(&animations); ++i)
            animations.at(&animations, i)
                ->setDefaultTarget(QQmlProperty(this, targets.first().property));
    }
    m_motion->transition(actions, transition, this);
}
void Popup::closeFromInput() { close(); }
void Popup::close() {
    m_interacting      = false;
    m_requestedVisible = false;
    if (! isVisible() || closing()) return;
    m_motion->cancel();
    QPointer<Popup> guard(this);
    if (! changeState(Closing)) return;
    if (m_overlay) m_overlay->updateDimmer(this);
    if (! guard || ! closing()) return;
    Q_EMIT aboutToHide();
    if (! guard || ! closing() || m_deferred) return;
    if (m_exit)
        startTransition(false);
    else if (m_hideDelay)
        m_hideTimer.start(m_hideDelay);
    else
        completeExit();
}
void Popup::completeEnter() {
    if (! entering()) return;
    QPointer<Popup> guard(this);
    finalizeTransition(true);
    if (! guard || ! entering()) return;
    if (changeState(Open)) Q_EMIT opened();
}
void Popup::completeExit() {
    if (! closing()) return;
    finishClose();
}
void Popup::finishClose() {
    QPointer<Popup> guard(this);
    m_hideTimer.stop();
    finalizeTransition(false);
    if (! guard || ! closing()) return;
    if (m_overlay) m_overlay->remove(this);
    if (! guard || ! closing()) return;
    m_surface->setVisible(false);
    m_surface->setParentItem(nullptr);
    // Enter/exit animations may leave the visual values at their terminal state.
    m_surface->setOpacity(m_savedOpacity);
    m_surface->setScale(m_savedScale);
    if (changeState(Closed)) Q_EMIT closed();
}
void Popup::dismissImmediately() {
    m_interacting      = false;
    m_requestedVisible = false;
    if (! isVisible() || m_dismissing) return;
    m_dismissing = true;
    m_motion->cancel();
    QPointer<Popup> guard(this);
    if (! closing()) {
        if (! changeState(Closing)) {
            if (guard) m_dismissing = false;
            return;
        }
        Q_EMIT aboutToHide();
        if (! guard) return;
    }
    m_motion->cancel();
    finishClose();
    if (guard) m_dismissing = false;
}
void Popup::forceActiveFocus(Qt::FocusReason reason) { m_surface->forceActiveFocus(reason); }
void Popup::reposition() {
    if (m_positioning || ! isVisible() || ! m_overlay || ! m_parent) return;
    QPointer<Popup> guard(this);
    m_positioning = true;
    m_surface->setPosition(surfacePosition());
    if (! guard) return;
    if (m_overlay) m_overlay->updateDimmer(this);
    if (guard) m_positioning = false;
}
void Popup::updateDimmer(QQuickItem* item, qreal opacity) const {
    QPointer<QQuickItem> guard(item);
    const qreal          targetOpacity = closing() ? 0 : opacity;
    const QSizeF         size(overlayWidth(), overlayHeight());
    item->setPosition({});
    if (guard) item->setSize(size);
    // Property writes trigger the dimmer's QML Behavior; direct setters do not.
    if (guard) QQmlProperty::write(item, QStringLiteral("opacity"), targetOpacity);
}
QPointF Popup::surfacePosition() const {
    if (! m_parent || ! m_overlay) return m_position;
    auto reference = m_positioningItem ? m_positioningItem.data() : m_parent.data();
    if (reference->window() != m_overlay->window()) return m_surface->position();
    QPointF position = reference->mapToItem(m_overlay, m_position);
    if (m_collision == Clamp) {
        const auto left = std::max<qreal>(0, leftMargin()), top = std::max<qreal>(0, topMargin());
        position.setX(std::clamp(
            position.x(),
            left,
            std::max(left, overlayWidth() - std::max<qreal>(0, rightMargin()) - width())));
        position.setY(std::clamp(
            position.y(),
            top,
            std::max(top, overlayHeight() - std::max<qreal>(0, bottomMargin()) - height())));
    }
    return position;
}
bool Popup::containsScenePoint(const QPointF& point) const {
    auto item = popupItem();
    return isVisible() && item && item->isVisible() && item->contains(item->mapFromScene(point));
}
bool Popup::parentContainsScenePoint(const QPointF& point) const {
    return m_parent && m_parent->contains(m_parent->mapFromScene(point));
}
void Popup::cancelOverlayPress() { m_outsidePressed = m_outsideParentPressed = false; }
void Popup::overlayPress(const QPointF& point) {
    m_outsidePressed       = ! containsScenePoint(point);
    m_outsideParentPressed = m_outsidePressed && ! parentContainsScenePoint(point);
    if (! closing() && m_outsidePressed &&
        (m_closePolicy.testFlag(CloseOnPressOutside) ||
         (m_outsideParentPressed && m_closePolicy.testFlag(CloseOnPressOutsideParent))))
        closeFromInput();
}
void Popup::overlayRelease(const QPointF& point) {
    const bool outside       = m_outsidePressed;
    const bool outsideParent = m_outsideParentPressed;
    cancelOverlayPress();
    if (! closing() && isVisible() && outside && overlayContainsScenePoint(point) &&
        ! containsScenePoint(point) &&
        (m_closePolicy.testFlag(CloseOnReleaseOutside) ||
         (outsideParent && ! parentContainsScenePoint(point) &&
          m_closePolicy.testFlag(CloseOnReleaseOutsideParent))))
        closeFromInput();
}
void Popup::setModal(bool value) {
    if (m_modal == value) return;
    const bool oldDim = dim();
    m_modal           = value;
    QPointer<Popup> guard(this);
    if (m_overlay) m_overlay->refresh();
    if (! guard) return;
    Q_EMIT modalChanged();
    if (guard && oldDim != dim()) Q_EMIT dimChanged();
}
void Popup::setDim(bool value) {
    const bool old = dim();
    m_dim          = value;
    if (m_overlay) m_overlay->refresh();
    if (old != dim()) Q_EMIT dimChanged();
}
void Popup::resetDim() {
    const bool old = dim();
    m_dim.reset();
    if (m_overlay) m_overlay->refresh();
    if (old != dim()) Q_EMIT dimChanged();
}
void Popup::setFocus(bool value) {
    if (m_focus == value) return;
    m_focus = value;
    Q_EMIT focusChanged();
}
void Popup::setDeferredCompletion(bool value) {
    if (m_deferred == value) return;
    m_deferred = value;
    Q_EMIT deferredCompletionChanged();
}
void Popup::setHideDelay(int value) {
    value = std::max(0, value);
    if (m_hideDelay == value) return;
    m_hideDelay = value;
    Q_EMIT hideDelayChanged();
}
void Popup::setClosePolicy(ClosePolicy value) {
    if (m_closePolicy == value) return;
    m_closePolicy = value;
    Q_EMIT closePolicyChanged();
}
void Popup::setCollisionPolicy(CollisionPolicy value) {
    if (m_collision == value) return;
    m_collision = value;
    reposition();
    Q_EMIT collisionPolicyChanged();
}
void Popup::setX(qreal value) {
    if (x() == value) return;
    m_position.setX(value);
    reposition();
    Q_EMIT xChanged();
}
void Popup::setY(qreal value) {
    if (y() == value) return;
    m_position.setY(value);
    reposition();
    Q_EMIT yChanged();
}
void Popup::setZ(qreal value) {
    if (m_z == value) return;
    m_z = value;
    if (m_overlay) m_overlay->refresh();
    Q_EMIT zChanged();
}
void Popup::setMargins(qreal value) {
    if (m_margins == value) return;
    m_margins = value;
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::setLeftMargin(qreal value) {
    m_leftMargin = value;
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::resetLeftMargin() {
    m_leftMargin.reset();
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::setRightMargin(qreal value) {
    m_rightMargin = value;
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::resetRightMargin() {
    m_rightMargin.reset();
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::setTopMargin(qreal value) {
    m_topMargin = value;
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::resetTopMargin() {
    m_topMargin.reset();
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::setBottomMargin(qreal value) {
    m_bottomMargin = value;
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::resetBottomMargin() {
    m_bottomMargin.reset();
    reposition();
    Q_EMIT marginsChanged();
}
void Popup::setEnter(QQuickTransition* value) {
    if (m_enter == value) return;
    m_enter = value;
    Q_EMIT enterChanged();
}
void Popup::setExit(QQuickTransition* value) {
    if (m_exit == value) return;
    m_exit = value;
    Q_EMIT exitChanged();
}
} // namespace qml_material
