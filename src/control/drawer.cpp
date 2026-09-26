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
    connect(this, &Popup::closed, this, [this] {
        clearExpansion(presentationOwner());
    });
    connect(this, &Popup::enabledChanged, this, [this] {
        if (! enabled()) cancelDrag();
    });
}
Drawer::~Drawer() {
    if (m_inputOverlay) m_inputOverlay->unregisterDrawer(this);
}
void Drawer::setRevealMode(RevealMode value) {
    if (value != Slide && value != Expand) return;
    if (m_revealMode == value) return;
    if (isVisible()) {
        qmlWarning(this) << "Cannot change revealMode while visible";
        return;
    }
    m_revealMode = value;
    surfaceItem()->setClip(value == Expand);
    QPointer<Drawer> guard(this);
    reposition();
    if (guard) Q_EMIT revealModeChanged();
}
bool Drawer::prepareExpansion(QObject* owner, ItemProxy* origin) {
    if (owner != presentationOwner() || ! origin || m_revealMode != Expand) return false;
    const auto geometry = origin->geometryIn(overlayItem());
    if (! geometry) return false;
    m_origin         = origin;
    m_originGeometry = *geometry;
    m_geometryReady  = false;
    QPointer<Drawer> guard(this);
    observeOrigin();
    return guard;
}
void Drawer::clearExpansion(QObject* owner) {
    if (owner != presentationOwner()) return;
    m_origin        = nullptr;
    m_geometryReady = false;
    observeOrigin();
}
void Drawer::observeOrigin() {
    for (const auto& connection : m_originConnections) disconnect(connection);
    m_originConnections.clear();
    for (auto* item = static_cast<QQuickItem*>(m_origin.data()); item; item = item->parentItem()) {
        for (auto signal : { &QQuickItem::xChanged,
                             &QQuickItem::yChanged,
                             &QQuickItem::widthChanged,
                             &QQuickItem::heightChanged,
                             &QQuickItem::scaleChanged,
                             &QQuickItem::rotationChanged })
            m_originConnections.append(connect(item, signal, this, &Drawer::reposition));
        m_originConnections.append(
            connect(item, &QQuickItem::transformOriginChanged, this, &Drawer::reposition));
        m_originConnections.append(
            connect(item, &QQuickItem::parentChanged, this, &Drawer::observeOrigin));
    }
    reposition();
}
ItemProxy::Geometry Drawer::originGeometry() const {
    if (m_origin) {
        const auto current = m_origin->geometryIn(overlayItem());
        if (current) return *current;
    }
    return m_originGeometry;
}
qreal Drawer::presentationScale() const {
    return m_revealMode == Expand && m_origin ? expansionGeometry().scale : 1;
}
QRectF              Drawer::expansionRect() const { return expansionGeometry().rect; }
ItemProxy::Geometry Drawer::expansionGeometry() const {
    const auto geometry = originGeometry();
    const auto origin   = geometry.rect.adjusted(-leftPadding() * geometry.scale,
                                                 -topPadding() * geometry.scale,
                                                 rightPadding() * geometry.scale,
                                                 bottomPadding() * geometry.scale);
    auto       end      = Popup::surfacePosition();
    switch (m_edge) {
    case Qt::LeftEdge: end.setX(origin.left()); break;
    case Qt::RightEdge: end.setX(origin.right() - width()); break;
    case Qt::TopEdge: end.setY(origin.top()); break;
    case Qt::BottomEdge: end.setY(origin.bottom() - height()); break;
    }
    const ItemProxy::Geometry start { origin, geometry.scale };
    const ItemProxy::Geometry target { QRectF(end, QSizeF(width(), height())), 1 };
    const auto mix = [](const ItemProxy::Geometry& a, const ItemProxy::Geometry& b, qreal p) {
        return ItemProxy::Geometry { QRectF(a.rect.topLeft() +
                                                (b.rect.topLeft() - a.rect.topLeft()) * p,
                                            a.rect.size() + (b.rect.size() - a.rect.size()) * p),
                                     std::lerp(a.scale, b.scale, p) };
    };
    const auto sample = [&] {
        if (m_anchorProgress > 0 && m_anchorProgress < 1) {
            if (m_position <= m_anchorProgress)
                return mix(m_startGeometry, m_anchorGeometry, m_position / m_anchorProgress);
            return mix(m_anchorGeometry,
                       m_endGeometry,
                       (m_position - m_anchorProgress) / (1 - m_anchorProgress));
        }
        return mix(m_startGeometry, m_endGeometry, m_position);
    };
    if (! m_geometryReady) {
        m_anchorProgress = 0;
        m_geometryReady  = true;
    } else if (start.rect != m_startGeometry.rect || start.scale != m_startGeometry.scale ||
               target.rect != m_endGeometry.rect) {
        // Keep the displayed geometry when either endpoint moves during a transition.
        m_anchorGeometry = sample();
        m_anchorProgress = m_position;
    }
    m_startGeometry = start;
    m_endGeometry   = target;
    return sample();
}
QSizeF Drawer::surfaceSize() const {
    if (m_revealMode == Expand && m_origin) return expansionRect().size() / presentationScale();
    return Popup::surfaceSize();
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
    if (m_revealMode == Expand && (entering() || closing())) return false;
    if (! canRequestPresentation()) return false;
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
    const auto size  = surfaceSize() * presentationScale();
    return m_edge == Qt::LeftEdge || m_edge == Qt::RightEdge
               ? local.y() >= 0 && local.y() < size.height()
               : local.x() >= 0 && local.x() < size.width();
}
void Drawer::pressDrag(const QPointF& point, ulong timestamp) {
    m_pressPoint     = point;
    m_pressTimestamp = timestamp;
    m_wasOpen        = isVisible() && ! closing();
}
bool Drawer::wantsDrag(const QPointF& point) const {
    if (m_revealMode == Expand && (entering() || closing())) return false;
    if (! canRequestPresentation()) return false;
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
    if (m_revealMode == Expand) {
        if (canRequestPresentation() && ! entering() && ! closing()) m_dragging = true;
        return;
    }
    if (! requestPresentation()) return;
    m_dragging     = true;
    m_dragOrigin   = axis(point);
    m_dragPosition = m_position;
    beginInteractiveTransition();
}
void Drawer::moveDrag(const QPointF& point) {
    if (m_revealMode == Expand) return;
    if (m_dragging && extent() > 0)
        setPosition(m_dragPosition + (axis(point) - m_dragOrigin) / extent());
}
void Drawer::releaseDrag(const QPointF& point, ulong timestamp) {
    if (! m_dragging) return;
    m_dragging        = false;
    const qreal delta = axis(point - m_pressPoint);
    if (m_revealMode == Expand) {
        const auto threshold = std::max(20, qGuiApp->styleHints()->startDragDistance() + 5);
        if (! m_wasOpen && delta > threshold)
            open();
        else if (m_wasOpen && delta < -threshold)
            close();
        return;
    }
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
    if (m_revealMode == Expand) return;
    endInteractiveTransition(m_wasOpen);
}
void Drawer::open() {
    if (! requestPresentation()) return;
    if (m_inputOverlay) m_inputOverlay->releaseDrawer(this);
    const bool wasDragging = m_dragging && m_revealMode == Slide;
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
    if (m_revealMode == Expand && m_origin) return expansionRect().topLeft();
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
    const auto size  = surfaceSize() * presentationScale();
    if (m_edge == Qt::LeftEdge || m_edge == Qt::RightEdge) {
        rect.setY(point.y());
        rect.setHeight(size.height());
    } else {
        rect.setX(point.x());
        rect.setWidth(size.width());
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
