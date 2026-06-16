#include "qml_material/control/popup.hpp"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QWheelEvent>
#include <algorithm>

using namespace qml_material;

Popup::Popup(QQuickItem* parent): QQuickItem(parent) {
    setVisible(false);
    setFocus(true);
    setFiltersChildMouseEvents(true);
    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, &Popup::finishClose);
    updateInputAcceptance();
}

bool Popup::isOpened() const { return m_opened; }
bool Popup::entering() const { return m_entering; }
bool Popup::closing() const { return m_closing; }

bool Popup::modal() const { return m_modal; }
void Popup::setModal(bool value) {
    if (m_modal == value) return;
    m_modal = value;
    updateInputAcceptance();
    emit modalChanged();
}

bool Popup::dim() const { return m_dim; }
void Popup::setDim(bool value) {
    if (m_dim == value) return;
    m_dim = value;
    emit dimChanged();
}

bool Popup::fullScreen() const { return m_fullScreen; }
void Popup::setFullScreen(bool value) {
    if (m_fullScreen == value) return;
    m_fullScreen = value;
    if (isVisible()) {
        if (m_fullScreen) {
            useOverlayParent();
        } else {
            restoreParentItem();
        }
    }
    emit fullScreenChanged();
}

bool Popup::visualFocus() const { return hasActiveFocus() && isVisualFocusReason(m_focusReason); }
Qt::FocusReason Popup::focusReason() const { return m_focusReason; }

Popup::ClosePolicy Popup::closePolicy() const { return m_closePolicy; }
void Popup::setClosePolicy(ClosePolicy value) {
    if (m_closePolicy == value) return;
    m_closePolicy = value;
    emit closePolicyChanged();
}

bool Popup::deferredCompletion() const { return m_deferredCompletion; }
void Popup::setDeferredCompletion(bool value) {
    if (m_deferredCompletion == value) return;
    m_deferredCompletion = value;
    emit deferredCompletionChanged();
}

int Popup::hideDelay() const { return m_hideDelay; }
void Popup::setHideDelay(int value) {
    value = std::max(0, value);
    if (m_hideDelay == value) return;
    m_hideDelay = value;
    emit hideDelayChanged();
}

QQuickItem* Popup::popupItem() const { return m_popupItem; }
void Popup::setPopupItem(QQuickItem* item) {
    if (m_popupItem == item) return;
    m_popupItem = item;
    emit popupItemChanged();
}

QQuickItem* Popup::overlayItem() const { return m_overlayItem; }
qreal Popup::overlayWidth() const { return m_overlayItem ? m_overlayItem->width() : 0; }
qreal Popup::overlayHeight() const { return m_overlayItem ? m_overlayItem->height() : 0; }

void Popup::open() {
    if ((m_opened || m_entering) && !m_closing) return;

    m_hideTimer.stop();
    setOpened(false);
    setEntering(true);
    setClosing(false);
    updateOverlayItem();
    if (m_fullScreen) useOverlayParent();
    setVisible(true);
    if (m_modal) {
        saveFocus();
        forceActiveFocus(Qt::PopupFocusReason);
        setFocusReason(Qt::PopupFocusReason);
    }
    updateInputAcceptance();
    emit aboutToShow();

    if (!m_deferredCompletion) completeEnter();
}

void Popup::close() {
    if (!m_opened && !m_entering && !m_closing && !isVisible()) return;
    if (m_closing) return;

    setEntering(false);
    setOpened(false);
    setClosing(true);
    m_outsidePressed       = false;
    m_outsideParentPressed = false;
    updateInputAcceptance();
    emit aboutToHide();

    if (m_deferredCompletion) return;

    if (m_hideDelay == 0) {
        completeExit();
    } else {
        m_hideTimer.start(m_hideDelay);
    }
}

void Popup::completeEnter() {
    if (!m_entering) return;

    setEntering(false);
    setOpened(true);
    updateInputAcceptance();
    emit opened();
}

void Popup::completeExit() {
    if (!m_closing) return;

    m_hideTimer.stop();
    setVisible(false);
    setEntering(false);
    setClosing(false);
    updateInputAcceptance();
    restoreFocus();
    restoreParentItem();
    emit closed();
}

void Popup::setEntering(bool value) {
    if (m_entering == value) return;
    const bool oldVisualFocus = visualFocus();
    m_entering = value;
    emit enteringChanged();
    if (oldVisualFocus != visualFocus()) emit visualFocusChanged();
}

void Popup::setOpened(bool value) {
    if (m_opened == value) return;
    m_opened = value;
    emit openedChanged();
}

void Popup::setClosing(bool value) {
    if (m_closing == value) return;
    m_closing = value;
    emit closingChanged();
}

void Popup::finishClose() {
    completeExit();
}

void Popup::updateInputAcceptance() {
    const bool blocksInput = m_modal && (m_opened || m_entering || m_closing);
    setAcceptedMouseButtons(blocksInput ? Qt::AllButtons : Qt::NoButton);
    setAcceptTouchEvents(blocksInput);
    setAcceptHoverEvents(blocksInput);
}

void Popup::saveFocus() {
    if (auto* win = window()) {
        auto* item = win->activeFocusItem();
        if (item && item != this) m_previousFocusItem = item;
    }
}

void Popup::restoreFocus() {
    if (!m_previousFocusItem) return;
    if (m_previousFocusItem->window() == window())
        m_previousFocusItem->forceActiveFocus(Qt::PopupFocusReason);
    m_previousFocusItem.clear();
}

void Popup::setFocusReason(Qt::FocusReason reason) {
    if (m_focusReason == reason) return;
    const bool oldVisualFocus = visualFocus();
    m_focusReason = reason;
    emit focusReasonChanged();
    if (oldVisualFocus != visualFocus()) emit visualFocusChanged();
}

bool Popup::isVisualFocusReason(Qt::FocusReason reason) const {
    return reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason ||
           reason == Qt::ShortcutFocusReason;
}

void Popup::updateOverlayItem() {
    auto* win = window();
    setOverlayItem(win ? win->contentItem() : nullptr);
}

void Popup::setOverlayItem(QQuickItem* item) {
    if (m_overlayItem == item) return;
    if (m_overlayItem) disconnect(m_overlayItem, nullptr, this, nullptr);
    m_overlayItem = item;
    if (m_overlayItem) {
        connect(m_overlayItem, &QQuickItem::widthChanged, this, &Popup::overlayGeometryChanged);
        connect(m_overlayItem, &QQuickItem::heightChanged, this, &Popup::overlayGeometryChanged);
        connect(m_overlayItem, &QObject::destroyed, this, [this] {
            m_overlayItem = nullptr;
            emit overlayItemChanged();
            emit overlayGeometryChanged();
        });
    }
    emit overlayItemChanged();
    emit overlayGeometryChanged();
}

void Popup::useOverlayParent() {
    updateOverlayItem();
    if (!m_overlayItem) return;

    if (!m_parentItemStored) {
        m_previousParentItem = parentItem();
        m_previousPosition   = position();
        m_parentItemStored   = true;
    }

    if (parentItem() != m_overlayItem) setParentItem(m_overlayItem);
    setPosition(QPointF(0, 0));
}

void Popup::restoreParentItem() {
    if (!m_parentItemStored) return;
    setParentItem(m_previousParentItem);
    setPosition(m_previousPosition);
    m_previousParentItem.clear();
    m_previousPosition   = QPointF();
    m_parentItemStored   = false;
}

bool Popup::containsPopup(const QPointF& point) const {
    if (!m_popupItem) return contains(point);
    if (!m_popupItem->isVisible()) return false;
    return m_popupItem->contains(m_popupItem->mapFromItem(this, point));
}

bool Popup::containsParent(const QPointF& point) const {
    auto* parent = m_parentItemStored ? m_previousParentItem.data() : parentItem();
    if (!parent) return false;
    return parent->contains(parent->mapFromItem(this, point));
}

bool Popup::tryClose(const QPointF& point, ClosePolicy flags) {
    if (!isVisible() || containsPopup(point)) return false;

    const bool onOutsidePress =
        flags.testFlag(CloseOnPressOutside) && m_closePolicy.testFlag(CloseOnPressOutside);
    const bool onOutsideRelease =
        flags.testFlag(CloseOnReleaseOutside) && m_closePolicy.testFlag(CloseOnReleaseOutside);
    const bool onOutsideParentPress =
        flags.testFlag(CloseOnPressOutsideParent) &&
        m_closePolicy.testFlag(CloseOnPressOutsideParent);
    const bool onOutsideParentRelease =
        flags.testFlag(CloseOnReleaseOutsideParent) &&
        m_closePolicy.testFlag(CloseOnReleaseOutsideParent);
    const bool onOutside = (onOutsidePress || onOutsideRelease) && m_outsidePressed;
    const bool onOutsideParent =
        (onOutsideParentPress || onOutsideParentRelease) && m_outsideParentPressed &&
        !containsParent(point);

    if (!onOutside && !onOutsideParent) return false;
    close();
    return true;
}

bool Popup::blockInput(const QPointF& point, QEvent* event) const {
    if (!m_modal || !isVisible()) return false;
    if (containsPopup(point)) return false;
    if (event) event->accept();
    return true;
}

bool Popup::handlePress(const QPointF& point, QEvent* event) {
    setFocusReason(Qt::MouseFocusReason);
    m_pressPoint           = point;
    m_outsidePressed       = !containsPopup(point);
    m_outsideParentPressed = m_outsidePressed && !containsParent(point);

    if (tryClose(point, CloseOnPressOutside | CloseOnPressOutsideParent)) {
        if (event) event->accept();
        return true;
    }
    return blockInput(point, event);
}

bool Popup::handleRelease(const QPointF& point, QEvent* event) {
    const bool hadOutsidePress = m_outsidePressed;
    const bool closed =
        hadOutsidePress && tryClose(point, CloseOnReleaseOutside | CloseOnReleaseOutsideParent);

    m_pressPoint           = QPointF();
    m_outsidePressed       = false;
    m_outsideParentPressed = false;

    if (closed) {
        if (event) event->accept();
        return true;
    }
    return blockInput(point, event);
}

void Popup::focusInEvent(QFocusEvent* event) {
    setFocusReason(event->reason());
    event->accept();
}

void Popup::focusOutEvent(QFocusEvent* event) {
    const bool oldVisualFocus = visualFocus();
    QQuickItem::focusOutEvent(event);
    event->accept();
    if (oldVisualFocus != visualFocus()) emit visualFocusChanged();
}

void Popup::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape && hasActiveFocus() && m_closePolicy.testFlag(CloseOnEscape)) {
        close();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) {
        setFocusReason(event->key() == Qt::Key_Tab ? Qt::TabFocusReason : Qt::BacktabFocusReason);
        event->accept();
        return;
    }
    QQuickItem::keyPressEvent(event);
}

void Popup::mousePressEvent(QMouseEvent* event) {
    if (handlePress(event->position(), event)) return;
    QQuickItem::mousePressEvent(event);
}

void Popup::mouseReleaseEvent(QMouseEvent* event) {
    if (handleRelease(event->position(), event)) return;
    QQuickItem::mouseReleaseEvent(event);
}

void Popup::wheelEvent(QWheelEvent* event) {
    if (m_modal && isVisible()) {
        event->accept();
        return;
    }
    QQuickItem::wheelEvent(event);
}

bool Popup::childMouseEventFilter(QQuickItem* item, QEvent* event) {
    if (!m_modal || !item) return QQuickItem::childMouseEventFilter(item, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        const auto point = item->mapToItem(this, mouse->position());
        if (handlePress(point, event)) return true;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        const auto point = item->mapToItem(this, mouse->position());
        if (handleRelease(point, event)) return true;
    }

    return QQuickItem::childMouseEventFilter(item, event);
}

void Popup::itemChange(ItemChange change, const ItemChangeData& value) {
    const bool oldVisualFocus = visualFocus();
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange) {
        updateOverlayItem();
        if (m_fullScreen && isVisible())
            useOverlayParent();
    } else if (change == ItemActiveFocusHasChanged) {
        if (oldVisualFocus != visualFocus()) emit visualFocusChanged();
    }
}

#include <qml_material/control/moc_popup.cpp>
