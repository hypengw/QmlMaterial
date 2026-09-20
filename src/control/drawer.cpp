#include "qml_material/control/drawer.hpp"
#include "qml_material/control/overlay.hpp"
#include <QGuiApplication>
#include <QStyleHints>
#include <QQmlInfo>
#include <algorithm>
#include <cmath>

namespace qml_material
{
Drawer::Drawer(QObject* parent): Popup(parent) {
    setModal(true);
    setFocus(true);
    setClosePolicy(CloseOnEscape | CloseOnReleaseOutside);
    resetDragMargin();
    connect(this, &Popup::overlayItemChanged, this, &Drawer::registerOverlay);
    connect(this, &Popup::enabledChanged, this, [this] {
        if (! enabled()) cancelDrag();
    });
}
Drawer::~Drawer() {
    if (m_inputOverlay) m_inputOverlay->unregisterDrawer(this);
}
void Drawer::registerOverlay() {
    auto next = qobject_cast<OverlayManager*>(overlayItem());
    if (next == m_inputOverlay) return;
    if (m_inputOverlay) m_inputOverlay->unregisterDrawer(this);
    m_inputOverlay = next;
    if (next) next->registerDrawer(this);
}
void Drawer::setInteractive(bool value) {
    if (m_interactive == value) return;
    m_interactive = value;
    QPointer<Drawer> guard(this);
    if (! value) cancelDrag();
    if (guard) Q_EMIT interactiveChanged();
}
void Drawer::setDragMargin(qreal value) {
    if (! std::isfinite(value) || m_dragMargin == value) return;
    m_dragMargin = value;
    Q_EMIT dragMarginChanged();
}
void  Drawer::resetDragMargin() { setDragMargin(qGuiApp->styleHints()->startDragDistance()); }
qreal Drawer::axis(const QPointF& point) const {
    switch (m_edge) {
    case Qt::LeftEdge: return point.x();
    case Qt::RightEdge: return -point.x();
    case Qt::TopEdge: return point.y();
    case Qt::BottomEdge: return -point.y();
    }
    return 0;
}
qreal Drawer::extent() const {
    return m_edge == Qt::LeftEdge || m_edge == Qt::RightEdge ? width() : height();
}
qreal Drawer::distanceFromEdge(const QPointF& point) const {
    switch (m_edge) {
    case Qt::LeftEdge: return point.x();
    case Qt::RightEdge: return overlayWidth() - point.x();
    case Qt::TopEdge: return point.y();
    case Qt::BottomEdge: return overlayHeight() - point.y();
    }
    return 0;
}
bool Drawer::acceptsDrag(const QPointF& point) const {
    if (! m_interactive || ! enabled() || ! parentItem() || ! parentItem()->isVisible() ||
        extent() <= 0)
        return false;
    if (! overlayContainsScenePoint(point)) return false;
    if (isVisible() && containsScenePoint(point)) return true;
    if (m_dragMargin <= 0) return false;
    const qreal distance = distanceFromEdge(point);
    return distance >= 0 && std::abs(distance - m_position * extent()) <= m_dragMargin;
}
bool Drawer::blocksScenePoint(const QPointF& point) const {
    return modal() && overlayContainsScenePoint(point);
}
bool Drawer::overlayContainsScenePoint(const QPointF& point) const {
    const auto local = point - surfacePosition();
    return m_edge == Qt::LeftEdge || m_edge == Qt::RightEdge
               ? local.y() >= 0 && local.y() < height()
               : local.x() >= 0 && local.x() < width();
}
void Drawer::pressDrag(const QPointF& point, ulong timestamp) {
    m_pressPoint     = point;
    m_pressTimestamp = timestamp;
    m_wasOpen        = isVisible() && ! closing();
}
bool Drawer::wantsDrag(const QPointF& point) const {
    if (! m_interactive || ! enabled() || extent() <= 0) return false;
    if (! isVisible() && m_dragMargin <= 0) return false;
    const auto delta  = point - m_pressPoint;
    const auto along  = axis(delta);
    const auto across = m_edge == Qt::LeftEdge || m_edge == Qt::RightEdge ? delta.y() : delta.x();
    const auto threshold = std::max(20, qGuiApp->styleHints()->startDragDistance() + 5);
    return std::abs(along) > threshold && std::abs(across) < threshold &&
           (isVisible() || along > 0);
}
void Drawer::startDrag(const QPointF& point) {
    m_dragging     = true;
    m_dragOrigin   = axis(point);
    m_dragPosition = m_position;
    beginInteractiveTransition();
}
void Drawer::moveDrag(const QPointF& point) {
    if (m_dragging && extent() > 0)
        setPosition(m_dragPosition + (axis(point) - m_dragOrigin) / extent());
}
void Drawer::releaseDrag(const QPointF& point, ulong timestamp) {
    if (! m_dragging) return;
    m_dragging           = false;
    const qreal delta    = axis(point - m_pressPoint);
    const auto  elapsed  = timestamp > m_pressTimestamp ? timestamp - m_pressTimestamp : 0;
    const qreal velocity = elapsed ? delta * 1000 / elapsed : 0;
    const bool  opening  = m_position > 0.7 || velocity > 300 ||
                           (! (m_position < 0.3 || velocity < -300) && delta > 0);
    endInteractiveTransition(opening);
}
void Drawer::cancelDrag() {
    if (m_inputOverlay) m_inputOverlay->releaseDrawer(this);
    if (! m_dragging) return;
    m_dragging = false;
    endInteractiveTransition(m_wasOpen);
}
void Drawer::open() {
    if (m_inputOverlay) m_inputOverlay->releaseDrawer(this);
    const bool wasDragging = m_dragging;
    m_dragging             = false;
    if (wasDragging)
        endInteractiveTransition(true);
    else
        Popup::open();
}
void Drawer::close() {
    if (m_inputOverlay) m_inputOverlay->releaseDrawer(this);
    m_dragging = false;
    Popup::close();
}
void Drawer::dismissImmediately() {
    if (m_inputOverlay) m_inputOverlay->releaseDrawer(this);
    m_dragging = false;
    Popup::dismissImmediately();
}
void Drawer::setEdge(Qt::Edge value) {
    if (value != Qt::LeftEdge && value != Qt::RightEdge && value != Qt::TopEdge &&
        value != Qt::BottomEdge) {
        qmlWarning(this) << "Invalid drawer edge";
        return;
    }
    if (m_edge == value) return;
    QPointer<Drawer> lifetime(this);
    cancelDrag();
    if (! lifetime) return;
    m_edge = value;
    QPointer<Drawer> guard(this);
    reposition();
    if (guard) Q_EMIT edgeChanged();
}
void Drawer::setPosition(qreal value) {
    if (! std::isfinite(value)) return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (m_position == value) return;
    m_position = value;
    QPointer<Drawer> guard(this);
    reposition();
    if (guard) Q_EMIT positionChanged();
}
QPointF Drawer::surfacePosition() const {
    auto point = Popup::surfacePosition();
    switch (m_edge) {
    case Qt::LeftEdge: point.setX((m_position - 1) * width()); break;
    case Qt::RightEdge: point.setX(overlayWidth() - m_position * width()); break;
    case Qt::TopEdge: point.setY((m_position - 1) * height()); break;
    case Qt::BottomEdge: point.setY(overlayHeight() - m_position * height()); break;
    }
    return point;
}
void Drawer::updateDimmer(QQuickItem* item, qreal) const {
    QRectF     rect(0, 0, overlayWidth(), overlayHeight());
    const auto point = surfacePosition();
    if (m_edge == Qt::LeftEdge || m_edge == Qt::RightEdge) {
        rect.setY(point.y());
        rect.setHeight(height());
    } else {
        rect.setX(point.x());
        rect.setWidth(width());
    }
    QPointer<QQuickItem> guard(item);
    const auto           opacity = m_position;
    item->setPosition(rect.topLeft());
    if (guard) item->setSize(rect.size());
    if (guard) item->setOpacity(opacity);
}
QList<Popup::TransitionTarget> Drawer::transitionTargets(bool opening) const {
    return { { QStringLiteral("position"), opening ? 1.0 : 0.0 } };
}
void Drawer::finalizeTransition(bool opening) { setPosition(opening ? 1 : 0); }
} // namespace qml_material
