#include "qml_material/scrollable/flickable.hpp"
#include "../input/scroll_input_p.hpp"

#include <QQuickWindow>
#include <QTimerEvent>
#include <QWindow>
#include <algorithm>
#include <cmath>

using namespace qml_material;

namespace
{

constexpr qreal Epsilon = 0.0001;

auto fuzzyLessThanOrEqualTo(qreal a, qreal b) -> bool {
    if (a == 0.0 || b == 0.0) {
        a += 1.0;
        b += 1.0;
    }
    return a <= b || qFuzzyCompare(a, b);
}

} // namespace

FlickableVisibleArea::FlickableVisibleArea(Flickable* parent)
    : QObject(parent), m_flickable(parent) {}

qreal FlickableVisibleArea::xPosition() const { return m_xPosition; }
qreal FlickableVisibleArea::yPosition() const { return m_yPosition; }
qreal FlickableVisibleArea::widthRatio() const { return m_widthRatio; }
qreal FlickableVisibleArea::heightRatio() const { return m_heightRatio; }

auto FlickableVisibleArea::updateVisible() -> void {
    if (! m_flickable) return;

    auto updateAxis = [](qreal  position,
                         qreal  minExtent,
                         qreal  maxExtent,
                         qreal  viewport,
                         qreal  content,
                         qreal  startMargin,
                         qreal  endMargin,
                         qreal& outPosition,
                         qreal& outRatio) -> void {
        const qreal total = std::max<qreal>(0, content + startMargin + endMargin);
        const qreal ratio = total <= 0 ? 1 : std::clamp(viewport / total, qreal(0), qreal(1));
        const qreal range = maxExtent - minExtent;
        const qreal normalized =
            range <= Epsilon ? 0 : (position - minExtent) / std::max<qreal>(range, Epsilon);
        outRatio    = ratio;
        outPosition = normalized * std::max<qreal>(0, 1 - ratio);
    };

    qreal xPosition = 0;
    qreal xRatio    = 1;
    qreal yPosition = 0;
    qreal yRatio    = 1;

    updateAxis(m_flickable->contentX(),
               m_flickable->minXExtent(),
               m_flickable->maxXExtent(),
               m_flickable->width(),
               m_flickable->contentWidth(),
               m_flickable->leftMargin(),
               m_flickable->rightMargin(),
               xPosition,
               xRatio);
    updateAxis(m_flickable->contentY(),
               m_flickable->minYExtent(),
               m_flickable->maxYExtent(),
               m_flickable->height(),
               m_flickable->contentHeight(),
               m_flickable->topMargin(),
               m_flickable->bottomMargin(),
               yPosition,
               yRatio);

    const bool xChanged      = ! qFuzzyCompare(m_xPosition, xPosition);
    const bool yChanged      = ! qFuzzyCompare(m_yPosition, yPosition);
    const bool widthChanged  = ! qFuzzyCompare(m_widthRatio, xRatio);
    const bool heightChanged = ! qFuzzyCompare(m_heightRatio, yRatio);
    m_xPosition              = xPosition;
    m_yPosition              = yPosition;
    m_widthRatio             = xRatio;
    m_heightRatio            = yRatio;
    QPointer<FlickableVisibleArea> guard(this);
    if (xChanged) emit xPositionChanged(m_xPosition);
    if (! guard) return;
    if (yChanged) emit yPositionChanged(m_yPosition);
    if (! guard) return;
    if (widthChanged) emit widthRatioChanged(m_widthRatio);
    if (! guard) return;
    if (heightChanged) emit heightRatioChanged(m_heightRatio);
}

Flickable::Flickable(QQuickItem* parent): QQuickItem(parent) {
    m_contentItem = new QQuickItem(this);
    m_contentItem->setObjectName(QStringLiteral("contentItem"));
    m_contentItem->setParentItem(this);

    m_visibleArea = new FlickableVisibleArea(this);

    ScrollInputAdapter::attach(this);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(m_acceptedButtons);
    setFlag(QQuickItem::ItemIsFocusScope, false);

    updateContentSize(HorizontalAxis);
    updateContentSize(VerticalAxis);
    updateBeginningEnd();
    m_motionClock.start();
    connect(this, &QQuickItem::windowChanged, this, &Flickable::attachMotionWindow);
    auto cancelUnavailable = [this] {
        if (! isVisible() || ! isEnabled()) {
            cancelScrollActivity();
        }
    };
    connect(this, &QQuickItem::visibleChanged, this, cancelUnavailable);
    connect(this, &QQuickItem::enabledChanged, this, cancelUnavailable);
    attachMotionWindow(window());
}

Flickable::~Flickable() {
    ScrollInputAdapter::cancel(this);
    disconnect(m_frameConnection);
    disconnect(m_windowVisibilityConnection);
    // QQuickItem emits window/visibility changes while its base destructor runs.
    disconnect(this, nullptr, this, nullptr);
}

auto Flickable::flickableData() -> QQmlListProperty<QObject> {
    return QQmlListProperty<QObject>(this,
                                     this,
                                     &Flickable::dataAppend,
                                     &Flickable::dataCount,
                                     &Flickable::dataAt,
                                     &Flickable::dataClear);
}

auto Flickable::flickableChildren() -> QQmlListProperty<QQuickItem> {
    return QQmlListProperty<QQuickItem>(this,
                                        this,
                                        &Flickable::childrenAppend,
                                        &Flickable::childrenCount,
                                        &Flickable::childrenAt,
                                        &Flickable::childrenClear);
}

auto Flickable::contentWidth() const -> qreal { return m_hData.viewSize; }

auto Flickable::setContentWidth(qreal value) -> void {
    updateContentGeometry({ value, contentHeight() });
}

auto Flickable::contentHeight() const -> qreal { return m_vData.viewSize; }

auto Flickable::setContentHeight(qreal value) -> void {
    updateContentGeometry({ contentWidth(), value });
}

auto Flickable::contentX() const -> qreal { return m_hData.position; }

auto Flickable::setContentX(qreal position) -> void {
    setContentPosition(HorizontalAxis, position);
}

auto Flickable::contentY() const -> qreal { return m_vData.position; }

auto Flickable::setContentY(qreal position) -> void { setContentPosition(VerticalAxis, position); }

void Flickable::setContentPosition(Axis axis, qreal position) {
    if (! std::isfinite(position)) return;
    QPointer<Flickable> guard(this);
    emit                scrollInterrupted();
    if (! guard) return;
    ScrollInputAdapter::cancel(this);
    if (! guard) return;
    endScroll(ScrollEnd::Cancelled);
    if (! guard) return;
    axisData(axis).platformScrolling = false;
    stopAxisMotion(axis);
    if (! guard) return;
    movementEnding();
    if (guard) setAxisPosition(axis, position);
}

auto Flickable::topMargin() const -> qreal { return m_vData.startMargin; }

auto Flickable::setTopMargin(qreal value) -> void {
    if (qFuzzyCompare(m_vData.startMargin, value)) return;
    m_vData.startMargin = value;
    if (! isDragging()) fixup(VerticalAxis);
    emit topMarginChanged();
    updateBeginningEnd();
}

auto Flickable::bottomMargin() const -> qreal { return m_vData.endMargin; }

auto Flickable::setBottomMargin(qreal value) -> void {
    if (qFuzzyCompare(m_vData.endMargin, value)) return;
    m_vData.endMargin = value;
    if (! isDragging()) fixup(VerticalAxis);
    emit bottomMarginChanged();
    updateBeginningEnd();
}

auto Flickable::leftMargin() const -> qreal { return m_hData.startMargin; }

auto Flickable::setLeftMargin(qreal value) -> void {
    if (qFuzzyCompare(m_hData.startMargin, value)) return;
    m_hData.startMargin = value;
    if (! isDragging()) fixup(HorizontalAxis);
    emit leftMarginChanged();
    updateBeginningEnd();
}

auto Flickable::rightMargin() const -> qreal { return m_hData.endMargin; }

auto Flickable::setRightMargin(qreal value) -> void {
    if (qFuzzyCompare(m_hData.endMargin, value)) return;
    m_hData.endMargin = value;
    if (! isDragging()) fixup(HorizontalAxis);
    emit rightMarginChanged();
    updateBeginningEnd();
}

auto Flickable::originY() const -> qreal { return 0; }
auto Flickable::originX() const -> qreal { return 0; }

auto Flickable::isMoving() const -> bool { return m_hData.moving || m_vData.moving; }
auto Flickable::isMovingHorizontally() const -> bool { return m_hData.moving; }
auto Flickable::isMovingVertically() const -> bool { return m_vData.moving; }
auto Flickable::isFlicking() const -> bool { return m_hData.flicking || m_vData.flicking; }
auto Flickable::isFlickingHorizontally() const -> bool { return m_hData.flicking; }
auto Flickable::isFlickingVertically() const -> bool { return m_vData.flicking; }
auto Flickable::isDragging() const -> bool { return m_hData.dragging || m_vData.dragging; }
auto Flickable::isDraggingHorizontally() const -> bool { return m_hData.dragging; }
auto Flickable::isDraggingVertically() const -> bool { return m_vData.dragging; }

auto Flickable::maximumFlickVelocity() const -> qreal { return m_maxVelocity; }

auto Flickable::setMaximumFlickVelocity(qreal value) -> void {
    value = std::max<qreal>(0, value);
    if (qFuzzyCompare(m_maxVelocity, value)) return;
    m_maxVelocity = value;
    emit maximumFlickVelocityChanged();
}

auto Flickable::flickDeceleration() const -> qreal { return m_deceleration; }

auto Flickable::setFlickDeceleration(qreal value) -> void {
    value = std::max<qreal>(1, value);
    if (qFuzzyCompare(m_deceleration, value)) return;
    m_deceleration = value;
    emit flickDecelerationChanged();
}

auto Flickable::isInteractive() const -> bool { return m_interactive; }

auto Flickable::setInteractive(bool value) -> void {
    if (m_interactive == value) return;
    m_interactive = value;
    QPointer<Flickable> guard(this);
    if (! m_interactive) {
        cancelScrollActivity();
    }
    if (guard) emit interactiveChanged();
}

auto Flickable::horizontalVelocity() const -> qreal { return m_hData.smoothVelocity; }
auto Flickable::verticalVelocity() const -> qreal { return m_vData.smoothVelocity; }
auto Flickable::isAtXEnd() const -> bool { return m_hData.atEnd; }
auto Flickable::isAtXBeginning() const -> bool { return m_hData.atBeginning; }
auto Flickable::isAtYEnd() const -> bool { return m_vData.atEnd; }
auto Flickable::isAtYBeginning() const -> bool { return m_vData.atBeginning; }
auto Flickable::contentItem() const -> QQuickItem* { return m_contentItem; }

auto Flickable::flickableDirection() const -> Flickable::FlickableDirection {
    return m_flickableDirection;
}

auto Flickable::setFlickableDirection(FlickableDirection direction) -> void {
    if (m_flickableDirection == direction) return;
    m_flickableDirection = direction;
    emit flickableDirectionChanged();
}

auto Flickable::pixelAligned() const -> bool { return m_pixelAligned; }

auto Flickable::setPixelAligned(bool align) -> void {
    if (m_pixelAligned == align) return;
    m_pixelAligned = align;
    setAxisPosition(HorizontalAxis, contentX());
    setAxisPosition(VerticalAxis, contentY());
    emit pixelAlignedChanged();
}

auto Flickable::synchronousDrag() const -> bool { return m_syncDrag; }

auto Flickable::setSynchronousDrag(bool value) -> void {
    if (m_syncDrag == value) return;
    m_syncDrag = value;
    emit synchronousDragChanged();
}

auto Flickable::acceptedButtons() const -> Qt::MouseButtons { return m_acceptedButtons; }

auto Flickable::setAcceptedButtons(Qt::MouseButtons buttons) -> void {
    if (m_acceptedButtons == buttons) return;
    m_acceptedButtons = buttons;
    setAcceptedMouseButtons(buttons);
    emit acceptedButtonsChanged();
}

auto Flickable::inputMaskMode() const -> Flickable::InputMaskMode { return m_inputMaskMode; }

auto Flickable::setInputMaskMode(InputMaskMode mode) -> void {
    if (m_inputMaskMode == mode) return;
    m_inputMaskMode = mode;
    emit inputMaskModeChanged();
}

auto Flickable::interactionItem() const -> QQuickItem* {
    return m_interactionItem ? m_interactionItem.data() : m_contentItem;
}

auto Flickable::setInteractionItem(QQuickItem* item) -> void {
    if (m_interactionItem == item) return;
    m_interactionItem = item;
    emit interactionItemChanged();
}

auto Flickable::resetInteractionItem() -> void { setInteractionItem(nullptr); }

auto Flickable::visibleArea() -> FlickableVisibleArea* { return m_visibleArea; }

void Flickable::resizeContent(qreal width, qreal height, QPointF center) {
    const qreal oldWidth  = vWidth();
    const qreal oldHeight = vHeight();
    QPointF     correction;
    if (! qFuzzyIsNull(center.x()) && ! qFuzzyIsNull(oldWidth))
        correction.setX(center.x() * width / oldWidth - center.x());
    if (! qFuzzyIsNull(center.y()) && ! qFuzzyIsNull(oldHeight))
        correction.setY(center.y() * height / oldHeight - center.y());
    updateContentGeometry({ width, height }, correction);
}

void Flickable::updateContentGeometry(QSizeF extent, QPointF anchorDelta) {
    if (! std::isfinite(extent.width()) || ! std::isfinite(extent.height()) ||
        ! std::isfinite(anchorDelta.x()) || ! std::isfinite(anchorDelta.y()))
        return;
    if (m_hData.viewSize != extent.width()) m_geometryNotifications |= 1;
    if (m_vData.viewSize != extent.height()) m_geometryNotifications |= 2;
    m_hData.viewSize = extent.width();
    m_vData.viewSize = extent.height();
    const QPointF before(contentX(), contentY());
    for (Axis axis : { HorizontalAxis, VerticalAxis }) {
        auto&       data       = axisData(axis);
        const qreal correction = axis == HorizontalAxis ? anchorDelta.x() : anchorDelta.y();
        const qreal desired    = data.position + correction;
        const qreal position   = boundedPosition(axis, aligned(desired));
        const qreal applied    = position - data.position;
        data.motion.translate(applied);
        if (position != data.position) m_geometryNotifications |= axis == HorizontalAxis ? 4 : 8;
        data.position = position;
        if (desired < minExtent(axis) || desired > maxExtent(axis)) data.motion.stop();
    }
    ++m_geometryRevision;
    const QPointer<Flickable> guard(this);
    const QPointF             adjusted = QPointF(contentX(), contentY()) - before;
    if (! adjusted.isNull()) emit scrollGeometryAdjusted(adjusted);
    if (guard) flushContentGeometry();
}

void Flickable::flushContentGeometry() {
    if (m_syncingGeometry) return;
    m_syncingGeometry = true;
    QPointer<Flickable> guard(this);
    for (;;) {
        const auto revision = m_geometryRevision;
        updateContentSize(HorizontalAxis);
        if (! guard) return;
        if (revision != m_geometryRevision) continue;
        updateContentSize(VerticalAxis);
        if (! guard) return;
        if (revision != m_geometryRevision) continue;
        m_contentItem->setPosition({ -contentX(), -contentY() });
        if (! guard) return;
        if (revision != m_geometryRevision) continue;
        updateBeginningEnd();
        if (! guard) return;
        if (revision != m_geometryRevision) continue;
        if (! m_geometryNotifications) break;
        const unsigned bit = m_geometryNotifications & (~m_geometryNotifications + 1);
        m_geometryNotifications &= ~bit;
        switch (bit) {
        case 1: emit contentWidthChanged(); break;
        case 2: emit contentHeightChanged(); break;
        case 4: emit contentXChanged(); break;
        case 8: emit contentYChanged(); break;
        }
        if (! guard) return;
        if (bit == 4 || bit == 8) viewportMoved(bit == 4 ? Qt::Horizontal : Qt::Vertical);
        if (! guard) return;
    }
    m_syncingGeometry = false;
    if (! m_hData.motion.active()) setAxisVelocity(HorizontalAxis, 0);
    if (! guard) return;
    if (! m_vData.motion.active()) setAxisVelocity(VerticalAxis, 0);
    if (! guard) return;
    movementEnding();
}

void Flickable::flick(qreal xVelocity, qreal yVelocity) {
    if (! std::isfinite(xVelocity) || ! std::isfinite(yVelocity)) return;
    QPointer<Flickable> guard(this);
    scrollInputStarted();
    if (! guard) return;
    ScrollInputAdapter::cancel(this);
    if (! guard) return;
    cancelFlick();
    if (! guard) return;
    bool flickedX = false;
    bool flickedY = false;
    if (xflick() && ! qFuzzyIsNull(xVelocity)) {
        startAxisFlick(HorizontalAxis, -xVelocity);
        flickedX = true;
    }
    if (yflick() && ! qFuzzyIsNull(yVelocity)) {
        startAxisFlick(VerticalAxis, -yVelocity);
        flickedY = true;
    }
    if (flickedX || flickedY) {
        movementStarting();
        flickingStarted(flickedX, flickedY);
        requestMotionFrame();
    }
}

void Flickable::cancelFlick() {
    QPointer<Flickable> guard(this);
    ScrollInputAdapter::cancelMotion(this);
    if (! guard) return;
    m_hData.platformScrolling = false;
    m_vData.platformScrolling = false;
    stopAxisMotion(HorizontalAxis);
    if (! guard) return;
    stopAxisMotion(VerticalAxis);
    if (! guard) return;
    movementEnding();
}

auto Flickable::dataAppend(QQmlListProperty<QObject>* property, QObject* object) -> void {
    if (! property || ! property->data || ! object) return;
    auto* flickable = static_cast<Flickable*>(property->data);
    if (! flickable->m_data.contains(object)) flickable->m_data.append(object);
    if (auto* item = qobject_cast<QQuickItem*>(object)) {
        item->setParent(flickable->m_contentItem);
        item->setParentItem(flickable->m_contentItem);
    } else {
        object->setParent(flickable);
    }
}

auto Flickable::dataCount(QQmlListProperty<QObject>* property) -> qsizetype {
    if (! property || ! property->data) return 0;
    return static_cast<Flickable*>(property->data)->m_data.size();
}

auto Flickable::dataAt(QQmlListProperty<QObject>* property, qsizetype index) -> QObject* {
    if (! property || ! property->data) return nullptr;
    const auto& data = static_cast<Flickable*>(property->data)->m_data;
    return index >= 0 && index < data.size() ? data.at(index) : nullptr;
}

auto Flickable::dataClear(QQmlListProperty<QObject>* property) -> void {
    if (! property || ! property->data) return;
    auto* flickable = static_cast<Flickable*>(property->data);
    for (auto* object : std::as_const(flickable->m_data)) {
        if (auto* item = qobject_cast<QQuickItem*>(object)) item->setParentItem(nullptr);
        object->setParent(nullptr);
    }
    flickable->m_data.clear();
}

auto Flickable::childrenAppend(QQmlListProperty<QQuickItem>* property, QQuickItem* item) -> void {
    if (! property || ! property->data || ! item) return;
    auto* flickable = static_cast<Flickable*>(property->data);
    item->setParent(flickable->m_contentItem);
    item->setParentItem(flickable->m_contentItem);
}

auto Flickable::childrenCount(QQmlListProperty<QQuickItem>* property) -> qsizetype {
    if (! property || ! property->data) return 0;
    return static_cast<Flickable*>(property->data)->m_contentItem->childItems().size();
}

auto Flickable::childrenAt(QQmlListProperty<QQuickItem>* property, qsizetype index) -> QQuickItem* {
    if (! property || ! property->data) return nullptr;
    const auto children = static_cast<Flickable*>(property->data)->m_contentItem->childItems();
    return index >= 0 && index < children.size() ? children.at(index) : nullptr;
}

auto Flickable::childrenClear(QQmlListProperty<QQuickItem>* property) -> void {
    if (! property || ! property->data) return;
    auto*      flickable = static_cast<Flickable*>(property->data);
    const auto children  = flickable->m_contentItem->childItems();
    for (auto* child : children) {
        child->setParentItem(nullptr);
        child->setParent(nullptr);
    }
}

auto Flickable::axisData(Axis axis) -> Flickable::AxisData& {
    return axis == HorizontalAxis ? m_hData : m_vData;
}

auto Flickable::axisData(Axis axis) const -> const Flickable::AxisData& {
    return axis == HorizontalAxis ? m_hData : m_vData;
}

auto Flickable::axisPosition(Axis axis) const -> qreal {
    return axis == HorizontalAxis ? contentX() : contentY();
}

auto Flickable::setAxisPosition(Axis axis, qreal position) -> void {
    if (! std::isfinite(position)) return;
    position                = boundedPosition(axis, aligned(position));
    const qreal oldPosition = axisPosition(axis);
    if (qFuzzyCompare(oldPosition, position)) {
        updateBeginningEnd();
        return;
    }

    axisData(axis).position = position;
    ++m_geometryRevision;
    m_geometryNotifications |= axis == HorizontalAxis ? 4 : 8;
    flushContentGeometry();
}

auto Flickable::minExtent(Axis axis) const -> qreal {
    return axis == HorizontalAxis ? minXExtent() : minYExtent();
}

auto Flickable::maxExtent(Axis axis) const -> qreal {
    return axis == HorizontalAxis ? maxXExtent() : maxYExtent();
}

auto Flickable::viewportSize(Axis axis) const -> qreal {
    return axis == HorizontalAxis ? width() : height();
}

auto Flickable::axisCanFlick(Axis axis) const -> bool {
    return axis == HorizontalAxis ? xflick() : yflick();
}

auto Flickable::aligned(qreal value) const -> qreal {
    if (! m_pixelAligned) return value;
    const auto* win   = window();
    const qreal ratio = win ? win->devicePixelRatio() : qGuiApp->devicePixelRatio();
    return std::round(value * ratio) / ratio;
}

auto Flickable::boundedPosition(Axis axis, qreal position) const -> qreal {
    return std::clamp(position, minExtent(axis), maxExtent(axis));
}

auto Flickable::updateContentSize(Axis axis) -> void {
    const auto& data = axisData(axis);
    const qreal size =
        data.viewSize < 0
            ? std::max<qreal>(0, viewportSize(axis) - data.startMargin - data.endMargin)
            : data.viewSize;
    if (axis == HorizontalAxis)
        m_contentItem->setWidth(size);
    else
        m_contentItem->setHeight(size);
}

auto Flickable::updateBeginningEnd() -> void {
    QPointer<Flickable> guard(this);
    auto                updateAxis = [this, &guard](Axis axis) -> bool {
        auto&       data            = axisData(axis);
        const qreal p               = axisPosition(axis);
        const bool  atBeginning     = fuzzyLessThanOrEqualTo(p, minExtent(axis));
        const bool  atEnd           = fuzzyLessThanOrEqualTo(maxExtent(axis), p);
        bool        boundaryChanged = false;

        if (data.atBeginning != atBeginning) {
            data.atBeginning = atBeginning;
            boundaryChanged  = true;
            if (axis == HorizontalAxis)
                emit atXBeginningChanged();
            else
                emit atYBeginningChanged();
            if (! guard) return true;
        }
        if (data.atEnd != atEnd) {
            data.atEnd      = atEnd;
            boundaryChanged = true;
            if (axis == HorizontalAxis)
                emit atXEndChanged();
            else
                emit atYEndChanged();
        }
        return boundaryChanged;
    };

    const bool hChanged = updateAxis(HorizontalAxis);
    if (! guard) return;
    const bool vChanged = updateAxis(VerticalAxis);
    if (! guard) return;
    if (hChanged || vChanged) emit isAtBoundaryChanged();
    if (! guard) return;
    updateVisibleArea();
}

auto Flickable::updateVisibleArea() -> void {
    if (m_visibleArea) m_visibleArea->updateVisible();
}

auto Flickable::setAxisMoving(Axis axis, bool value) -> void {
    auto& data = axisData(axis);
    if (data.moving == value) return;
    const bool wasMoving = isMoving();
    data.moving          = value;
    if (axis == HorizontalAxis)
        emit movingHorizontallyChanged();
    else
        emit movingVerticallyChanged();
    if (wasMoving != isMoving()) emit movingChanged();
}

auto Flickable::setAxisDragging(Axis axis, bool value) -> void {
    auto& data = axisData(axis);
    if (data.dragging == value) return;
    const bool wasDragging = isDragging();
    data.dragging          = value;
    if (axis == HorizontalAxis)
        emit draggingHorizontallyChanged();
    else
        emit draggingVerticallyChanged();
    if (wasDragging != isDragging()) emit draggingChanged();
}

auto Flickable::setAxisFlicking(Axis axis, bool value) -> void {
    auto& data = axisData(axis);
    if (data.flicking == value) return;
    const bool wasFlicking = isFlicking();
    data.flicking          = value;
    if (axis == HorizontalAxis)
        emit flickingHorizontallyChanged();
    else
        emit flickingVerticallyChanged();
    if (wasFlicking != isFlicking()) emit flickingChanged();
}

auto Flickable::setAxisVelocity(Axis axis, qreal value) -> void {
    auto& data = axisData(axis);
    if (qFuzzyCompare(data.smoothVelocity, value)) return;
    data.smoothVelocity = value;
    if (axis == HorizontalAxis)
        emit horizontalVelocityChanged();
    else
        emit verticalVelocityChanged();
}

auto Flickable::movementStarting() -> void {
    const bool wasMoving = isMoving();
    if (m_hData.motion.active() || m_hData.dragging || m_hData.platformScrolling)
        setAxisMoving(HorizontalAxis, true);
    if (m_vData.motion.active() || m_vData.dragging || m_vData.platformScrolling)
        setAxisMoving(VerticalAxis, true);
    if (! wasMoving && isMoving()) emit movementStarted();
}

auto Flickable::movementEnding() -> void {
    const bool wasMoving   = isMoving();
    const bool wasFlicking = isFlicking();

    if (! m_hData.motion.active() && ! m_hData.dragging && ! m_hData.platformScrolling)
        setAxisMoving(HorizontalAxis, false);
    if (! m_vData.motion.active() && ! m_vData.dragging && ! m_vData.platformScrolling)
        setAxisMoving(VerticalAxis, false);
    if (! m_hData.motion.active()) setAxisFlicking(HorizontalAxis, false);
    if (! m_vData.motion.active()) setAxisFlicking(VerticalAxis, false);

    if (wasFlicking && ! isFlicking()) emit flickEnded();
    if (wasMoving && ! isMoving()) emit movementEnded();
}

auto Flickable::flickingStarted(bool horizontal, bool vertical) -> void {
    const bool wasFlicking = isFlicking();
    if (horizontal) setAxisFlicking(HorizontalAxis, true);
    if (vertical) setAxisFlicking(VerticalAxis, true);
    if (! wasFlicking && isFlicking()) emit flickStarted();
}

auto Flickable::draggingStarting(bool horizontal, bool vertical) -> void {
    const bool wasDragging = isDragging();
    if (horizontal) setAxisDragging(HorizontalAxis, true);
    if (vertical) setAxisDragging(VerticalAxis, true);
    if (! wasDragging && isDragging()) emit dragStarted();
}

auto Flickable::setDragVelocity(QPointF velocity) -> void {
    velocity.setX(xflick() ? std::clamp(velocity.x(), -m_maxVelocity, m_maxVelocity) : 0);
    velocity.setY(yflick() ? std::clamp(velocity.y(), -m_maxVelocity, m_maxVelocity) : 0);
    if (velocity.isNull())
        m_dragVelocityTimer.stop();
    else
        m_dragVelocityTimer.start(100, Qt::PreciseTimer, this);
    if (m_dragVelocity == velocity) return;
    m_dragVelocity = velocity;
    emit dragVelocityChanged();
}

auto Flickable::draggingEnding(std::optional<QPointF> releaseVelocity) -> void {
    QPointer<Flickable> guard(this);
    const bool          wasDragging = isDragging();
    if (releaseVelocity) setDragVelocity(*releaseVelocity);
    if (! guard) return;
    setAxisDragging(HorizontalAxis, false);
    if (! guard) return;
    setAxisDragging(VerticalAxis, false);
    if (! guard) return;
    if (wasDragging && releaseVelocity) emit dragReleased(m_dragVelocity);
    if (! guard) return;
    if (wasDragging) emit dragEnded();
    if (guard) setDragVelocity({});
}

auto Flickable::startAxisFlick(Axis axis, qreal velocity) -> void {
    if (qFuzzyIsNull(velocity)) {
        fixup(axis);
        return;
    }

    auto& data = axisData(axis);
    velocity   = std::clamp(velocity, -m_maxVelocity, m_maxVelocity);
    data.motion.fling(
        axisPosition(axis), velocity, m_deceleration, m_motionClock.nsecsElapsed() / 1e9);
    setAxisVelocity(axis, velocity);
}

auto Flickable::stopAxisMotion(Axis axis) -> void {
    auto& data = axisData(axis);
    data.motion.stop();
    setAxisVelocity(axis, 0);
}

auto Flickable::attachMotionWindow(QQuickWindow* target) -> void {
    const bool replacing   = m_motionWindowAttached;
    m_motionWindowAttached = target != nullptr;
    disconnect(m_frameConnection);
    disconnect(m_windowVisibilityConnection);
    m_frameConnection            = {};
    m_windowVisibilityConnection = {};
    if (target) {
        m_frameConnection =
            connect(target, &QQuickWindow::afterAnimating, this, &Flickable::requestMotionFrame);
        m_windowVisibilityConnection =
            connect(target, &QWindow::visibleChanged, this, [this](bool visible) {
                if (! visible) cancelScrollActivity();
            });
    }
    if (replacing) cancelScrollActivity();
}

void Flickable::cancelScrollActivity() {
    QPointer<Flickable> guard(this);
    ScrollInputAdapter::cancel(this);
    if (! guard) return;
    cancelFlick();
    if (guard) scrollActivityCancelled();
}

auto Flickable::requestMotionFrame() -> void {
    if (! isAxisAnimating(HorizontalAxis) && ! isAxisAnimating(VerticalAxis)) return;
    if (! window() || ! isVisible() || ! isEnabled() || ! isInteractive()) {
        cancelScrollActivity();
        return;
    }
    polish();
    window()->update();
}

auto Flickable::updatePolish() -> void {
    QPointer<Flickable> guard(this);
    const qreal         now = m_motionClock.nsecsElapsed() / 1e9;
    advanceAxis(HorizontalAxis, now);
    if (! guard) return;
    advanceAxis(VerticalAxis, now);
    if (! guard) return;
    movementEnding();
}

auto Flickable::advanceAxis(Axis axis, qreal now) -> void {
    auto& data = axisData(axis);
    if (! data.motion.active()) return;
    const auto          sample   = data.motion.sample(now);
    const qreal         position = boundedPosition(axis, sample.position);
    const bool          finished = sample.finished || position != sample.position;
    const auto          revision = data.motion.revision();
    QPointer<Flickable> guard(this);
    setAxisPosition(axis, position);
    if (! guard || revision != data.motion.revision()) return;
    setAxisVelocity(axis, finished ? 0 : sample.velocity);
    if (! guard || revision != data.motion.revision()) return;
    if (finished) stopAxisMotion(axis);
}

auto Flickable::fixup(Axis axis) -> void { setAxisPosition(axis, axisPosition(axis)); }

auto Flickable::isAxisAnimating(Axis axis) const -> bool { return axisData(axis).motion.active(); }

Qt::Orientations Flickable::scrollAxes() const {
    Qt::Orientations result;
    if (xflick()) result |= Qt::Horizontal;
    if (yflick()) result |= Qt::Vertical;
    return result;
}
QPointF Flickable::boundedScrollPosition(QPointF p) const {
    return { boundedPosition(HorizontalAxis, p.x()), boundedPosition(VerticalAxis, p.y()) };
}
bool Flickable::acceptsScrollPoint(QPointF point) const {
    if (! contains(point)) return false;
    if (m_inputMaskMode == Viewport) return true;
    auto* item = m_inputMaskMode == CustomItem ? interactionItem() : m_contentItem;
    return item && item->isVisible() && item->isEnabled() &&
           item->contains(item->mapFromItem(this, point));
}
void Flickable::beginScroll(ScrollSource) {
    const QPointer<Flickable> guard(this);
    endScroll(ScrollEnd::Cancelled);
    if (! guard) return;
    const auto revision = ++m_activityRevision;
    scrollInputStarted();
    if (! guard) return;
    stopAxisMotion(HorizontalAxis);
    if (! guard || revision != m_activityRevision) return;
    stopAxisMotion(VerticalAxis);
    if (! guard || revision != m_activityRevision) return;
    movementEnding();
    if (guard && revision == m_activityRevision) m_scrollActive = true;
}
void Flickable::updateScrollActivity(ScrollSource source, Qt::Orientations axes, QPointF velocity) {
    const QPointer<Flickable> guard(this);
    const auto                revision = m_activityRevision;
    const auto                current  = [&] {
        return guard && revision == m_activityRevision;
    };
    const bool x = axes.testFlag(Qt::Horizontal), y = axes.testFlag(Qt::Vertical);
    if (source == ScrollSource::Drag) draggingStarting(x, y);
    if (! current()) return;
    if (source == ScrollSource::Fling) flickingStarted(x, y);
    if (! current()) return;
    if (x) m_hData.platformScrolling = true;
    if (y) m_vData.platformScrolling = true;
    movementStarting();
    if (! current()) return;
    setAxisVelocity(HorizontalAxis, x ? velocity.x() : 0);
    if (! current()) return;
    setAxisVelocity(VerticalAxis, y ? velocity.y() : 0);
}
void Flickable::releaseScroll(QPointF velocity) { draggingEnding(velocity); }
void Flickable::endScroll(ScrollEnd) {
    ++m_activityRevision;
    if (! m_scrollActive) return;
    m_scrollActive = false;
    const QPointer<Flickable> guard(this);
    m_hData.platformScrolling = false;
    m_vData.platformScrolling = false;
    draggingEnding();
    if (! guard) return;
    setAxisVelocity(HorizontalAxis, 0);
    if (! guard) return;
    setAxisVelocity(VerticalAxis, 0);
    if (guard) movementEnding();
}
Flickable::ScrollConsumption Flickable::applyScrollDelta(QPointF delta, ScrollSource source,
                                                         QPointF velocity) {
    ScrollConsumption result { {}, delta };
    if (! m_scrollActive || ! m_interactive || ! isVisible() || ! isEnabled() ||
        ! std::isfinite(delta.x()) || ! std::isfinite(delta.y()))
        return result;
    const QPointer<Flickable> guard(this);
    const auto                revision = m_activityRevision;
    const auto                current  = [&] {
        return guard && revision == m_activityRevision;
    };
    const QPointF before(contentX(), contentY());
    auto          after = boundedScrollPosition(before + delta);
    const auto    axes  = scrollAxes();
    if (! axes.testFlag(Qt::Horizontal)) after.setX(before.x());
    if (! axes.testFlag(Qt::Vertical)) after.setY(before.y());
    if (source == ScrollSource::Drag && (after != before || isDragging())) {
        setDragVelocity(velocity);
        if (! current()) return result;
    }
    if (after == before) return result;
    setAxisPosition(HorizontalAxis, after.x());
    if (! current()) return result;
    result.consumed.setX(contentX() - before.x());
    result.remaining = delta - result.consumed;
    setAxisPosition(VerticalAxis, after.y());
    if (! current()) return result;
    result.consumed.setY(contentY() - before.y());
    result.remaining = delta - result.consumed;
    Qt::Orientations moved;
    if (result.consumed.x()) moved |= Qt::Horizontal;
    if (result.consumed.y()) moved |= Qt::Vertical;
    updateScrollActivity(source, moved, velocity);
    return result;
}
Flickable::ScrollConsumption Flickable::consumeScroll(QPointF delta, ScrollInput input,
                                                      Qt::ScrollPhase phase) {
    return ScrollInputAdapter::scroll(this, delta, input, phase);
}

auto Flickable::timerEvent(QTimerEvent* event) -> void {
    if (event->timerId() == m_dragVelocityTimer.timerId()) {
        setDragVelocity({});
        return;
    }
    QQuickItem::timerEvent(event);
}

auto Flickable::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) -> void {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (! qFuzzyCompare(newGeometry.width(), oldGeometry.width()))
        updateContentSize(HorizontalAxis);
    if (! qFuzzyCompare(newGeometry.height(), oldGeometry.height()))
        updateContentSize(VerticalAxis);
    if (! isDragging()) {
        fixup(HorizontalAxis);
        fixup(VerticalAxis);
    }
    updateBeginningEnd();
}

auto Flickable::componentComplete() -> void {
    QQuickItem::componentComplete();
    updateContentSize(HorizontalAxis);
    updateContentSize(VerticalAxis);
    fixup(HorizontalAxis);
    fixup(VerticalAxis);
    updateBeginningEnd();
}

auto Flickable::minXExtent() const -> qreal { return originX() - m_hData.startMargin; }
auto Flickable::minYExtent() const -> qreal { return originY() - m_vData.startMargin; }

auto Flickable::maxXExtent() const -> qreal {
    return std::max(minXExtent(), vWidth() + m_hData.endMargin + originX() - width());
}

auto Flickable::maxYExtent() const -> qreal {
    return std::max(minYExtent(), vHeight() + m_vData.endMargin + originY() - height());
}

auto Flickable::vWidth() const -> qreal {
    return m_hData.viewSize < 0 ? std::max<qreal>(0, width() - leftMargin() - rightMargin())
                                : m_hData.viewSize;
}

auto Flickable::vHeight() const -> qreal {
    return m_vData.viewSize < 0 ? std::max<qreal>(0, height() - topMargin() - bottomMargin())
                                : m_vData.viewSize;
}

auto Flickable::viewportMoved(Qt::Orientations orientation) -> void { Q_UNUSED(orientation) }
void Flickable::scrollInputStarted() {}
void Flickable::scrollActivityCancelled() {}
void Flickable::setVerticalScrollTarget(qreal position) { setScrollTarget(VerticalAxis, position); }
void Flickable::setHorizontalScrollTarget(qreal position) {
    setScrollTarget(HorizontalAxis, position);
}
void Flickable::setScrollTarget(Axis axis, qreal position) {
    if (! std::isfinite(position)) return;
    position     = boundedPosition(axis, position);
    auto& data   = axisData(axis);
    auto& motion = data.motion;
    if (motion.mode() == ScrollMotion::Mode::Target && motion.target() == position) return;
    if (! window() || (qAbs(position - data.position) < 0.01 && qAbs(data.smoothVelocity) < 0.1)) {
        stopAxisMotion(axis);
        setAxisPosition(axis, position);
        movementEnding();
        return;
    }
    const qreal now      = m_motionClock.nsecsElapsed() / 1e9;
    const qreal velocity = motion.active() ? motion.sample(now).velocity : 0;
    motion.smooth(data.position, velocity, position, now);
    movementStarting();
    requestMotionFrame();
}

auto Flickable::xflick() const -> bool {
    const qreal contentWidthWithMargins = vWidth() + m_hData.startMargin + m_hData.endMargin;
    if ((m_flickableDirection & AutoFlickIfNeeded) && contentWidthWithMargins > width())
        return true;
    if (m_flickableDirection == AutoFlickDirection)
        return std::floor(std::abs(contentWidthWithMargins - width())) > 0;
    return m_flickableDirection & HorizontalFlick;
}

auto Flickable::yflick() const -> bool {
    const qreal contentHeightWithMargins = vHeight() + m_vData.startMargin + m_vData.endMargin;
    if ((m_flickableDirection & AutoFlickIfNeeded) && contentHeightWithMargins > height())
        return true;
    if (m_flickableDirection == AutoFlickDirection)
        return std::floor(std::abs(contentHeightWithMargins - height())) > 0;
    return m_flickableDirection & VerticalFlick;
}
