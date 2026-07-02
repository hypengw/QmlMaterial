#include "qml_material/flickable/flickable.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTimerEvent>
#include <QTouchEvent>
#include <QWheelEvent>
#include <QWindow>
#include <algorithm>
#include <cmath>

using namespace qml_material;

namespace
{

constexpr qreal MinimumFlickVelocity = 75.0;
constexpr qreal Epsilon              = 0.0001;

auto fuzzyLessThanOrEqualTo(qreal a, qreal b) -> bool {
    if (a == 0.0 || b == 0.0) {
        a += 1.0;
        b += 1.0;
    }
    return a <= b || qFuzzyCompare(a, b);
}

auto eventTimestamp(QInputEvent* event) -> qint64 {
    if (event && event->timestamp() != 0) return static_cast<qint64>(event->timestamp());
    return QDateTime::currentMSecsSinceEpoch();
}

auto isMouseEvent(QEvent* event) -> bool {
    return event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove ||
           event->type() == QEvent::MouseButtonRelease ||
           event->type() == QEvent::MouseButtonDblClick;
}

auto isTouchEvent(QEvent* event) -> bool {
    return event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate ||
           event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel;
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

    if (! qFuzzyCompare(m_xPosition, xPosition)) {
        m_xPosition = xPosition;
        emit xPositionChanged(m_xPosition);
    }
    if (! qFuzzyCompare(m_yPosition, yPosition)) {
        m_yPosition = yPosition;
        emit yPositionChanged(m_yPosition);
    }
    if (! qFuzzyCompare(m_widthRatio, xRatio)) {
        m_widthRatio = xRatio;
        emit widthRatioChanged(m_widthRatio);
    }
    if (! qFuzzyCompare(m_heightRatio, yRatio)) {
        m_heightRatio = yRatio;
        emit heightRatioChanged(m_heightRatio);
    }
}

auto Flickable::AxisData::resetDrag() -> void {
    velocityBuffer.clear();
    dragStartOffset   = 0;
    previousDragDelta = 0;
    pressPos          = 0;
    pressContentPos   = 0;
    lastPos           = 0;
}

auto Flickable::AxisData::addVelocitySample(qint64 timestamp, qreal position, qreal maxVelocity)
    -> void {
    velocityBuffer.append({ timestamp, position });
    while (velocityBuffer.size() > 10) {
        velocityBuffer.removeFirst();
    }
    while (velocityBuffer.size() > 2 && timestamp - velocityBuffer.first().first > 140) {
        velocityBuffer.removeFirst();
    }
    updateVelocity();
    velocity = std::clamp(velocity, -maxVelocity, maxVelocity);
}

auto Flickable::AxisData::updateVelocity() -> void {
    if (velocityBuffer.size() < 2) {
        velocity = 0;
        return;
    }

    const auto  first = velocityBuffer.first();
    const auto  last  = velocityBuffer.last();
    const qreal dt    = qreal(last.first - first.first) / 1000.0;
    if (dt <= 0) {
        velocity = 0;
        return;
    }
    velocity = (last.second - first.second) / dt;
}

Flickable::Flickable(QQuickItem* parent): QQuickItem(parent) {
    m_contentItem = new QQuickItem(this);
    m_contentItem->setObjectName(QStringLiteral("contentItem"));
    m_contentItem->setParentItem(this);

    m_visibleArea = new FlickableVisibleArea(this);

    setFiltersChildMouseEvents(true);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(m_acceptedButtons);
    setFlag(QQuickItem::ItemIsFocusScope, false);

    updateContentSize(HorizontalAxis);
    updateContentSize(VerticalAxis);
    updateBeginningEnd();
}

Flickable::~Flickable() = default;

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
    if (qFuzzyCompare(m_hData.viewSize, value)) return;
    m_hData.viewSize = value;
    updateContentSize(HorizontalAxis);
    if (! m_pressed && ! m_hData.moving) fixup(HorizontalAxis);
    emit contentWidthChanged();
    updateBeginningEnd();
}

auto Flickable::contentHeight() const -> qreal { return m_vData.viewSize; }

auto Flickable::setContentHeight(qreal value) -> void {
    if (qFuzzyCompare(m_vData.viewSize, value)) return;
    m_vData.viewSize = value;
    updateContentSize(VerticalAxis);
    if (! m_pressed && ! m_vData.moving) fixup(VerticalAxis);
    emit contentHeightChanged();
    updateBeginningEnd();
}

auto Flickable::contentX() const -> qreal { return -m_contentItem->x(); }

auto Flickable::setContentX(qreal position) -> void {
    stopAxisMotion(HorizontalAxis);
    if (isMoving() || isFlicking()) movementEnding();
    setAxisPosition(HorizontalAxis, position);
}

auto Flickable::contentY() const -> qreal { return -m_contentItem->y(); }

auto Flickable::setContentY(qreal position) -> void {
    stopAxisMotion(VerticalAxis);
    if (isMoving() || isFlicking()) movementEnding();
    setAxisPosition(VerticalAxis, position);
}

auto Flickable::topMargin() const -> qreal { return m_vData.startMargin; }

auto Flickable::setTopMargin(qreal value) -> void {
    if (qFuzzyCompare(m_vData.startMargin, value)) return;
    m_vData.startMargin = value;
    if (! m_pressed) fixup(VerticalAxis);
    emit topMarginChanged();
    updateBeginningEnd();
}

auto Flickable::bottomMargin() const -> qreal { return m_vData.endMargin; }

auto Flickable::setBottomMargin(qreal value) -> void {
    if (qFuzzyCompare(m_vData.endMargin, value)) return;
    m_vData.endMargin = value;
    if (! m_pressed) fixup(VerticalAxis);
    emit bottomMarginChanged();
    updateBeginningEnd();
}

auto Flickable::leftMargin() const -> qreal { return m_hData.startMargin; }

auto Flickable::setLeftMargin(qreal value) -> void {
    if (qFuzzyCompare(m_hData.startMargin, value)) return;
    m_hData.startMargin = value;
    if (! m_pressed) fixup(HorizontalAxis);
    emit leftMarginChanged();
    updateBeginningEnd();
}

auto Flickable::rightMargin() const -> qreal { return m_hData.endMargin; }

auto Flickable::setRightMargin(qreal value) -> void {
    if (qFuzzyCompare(m_hData.endMargin, value)) return;
    m_hData.endMargin = value;
    if (! m_pressed) fixup(HorizontalAxis);
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

auto Flickable::pressDelay() const -> int { return m_pressDelay; }

auto Flickable::setPressDelay(int delay) -> void {
    delay = std::max(0, delay);
    if (m_pressDelay == delay) return;
    m_pressDelay = delay;
    emit pressDelayChanged();
}

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
    if (! m_interactive) cancelInteraction();
    emit interactiveChanged();
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
    m_hData.viewSize      = width;
    m_vData.viewSize      = height;
    updateContentSize(HorizontalAxis);
    updateContentSize(VerticalAxis);
    emit contentWidthChanged();
    emit contentHeightChanged();

    if (! qFuzzyIsNull(center.x()) && ! qFuzzyIsNull(oldWidth))
        setAxisPosition(HorizontalAxis, contentX() + center.x() * width / oldWidth - center.x());
    if (! qFuzzyIsNull(center.y()) && ! qFuzzyIsNull(oldHeight))
        setAxisPosition(VerticalAxis, contentY() + center.y() * height / oldHeight - center.y());
    updateBeginningEnd();
}

void Flickable::flick(qreal xVelocity, qreal yVelocity) {
    m_hData.resetDrag();
    m_vData.resetDrag();
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
        ensureMotionTimer();
    }
}

void Flickable::cancelFlick() {
    stopAxisMotion(HorizontalAxis);
    stopAxisMotion(VerticalAxis);
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
    position                = aligned(boundedPosition(axis, position));
    const qreal oldPosition = axisPosition(axis);
    if (qFuzzyCompare(oldPosition, position)) {
        updateBeginningEnd();
        return;
    }

    if (axis == HorizontalAxis) {
        m_contentItem->setX(-position);
        emit contentXChanged();
        viewportMoved(Qt::Horizontal);
    } else {
        m_contentItem->setY(-position);
        emit contentYChanged();
        viewportMoved(Qt::Vertical);
    }
    updateBeginningEnd();
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
    auto updateAxis = [this](Axis axis) -> bool {
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
    const bool vChanged = updateAxis(VerticalAxis);
    if (hChanged || vChanged) emit isAtBoundaryChanged();
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
    if (m_hData.motionMode != NoMotion || m_hData.dragging) setAxisMoving(HorizontalAxis, true);
    if (m_vData.motionMode != NoMotion || m_vData.dragging) setAxisMoving(VerticalAxis, true);
    if (! wasMoving && isMoving()) emit movementStarted();
}

auto Flickable::movementEnding() -> void {
    const bool wasMoving   = isMoving();
    const bool wasFlicking = isFlicking();

    if (m_hData.motionMode == NoMotion && ! m_hData.dragging) setAxisMoving(HorizontalAxis, false);
    if (m_vData.motionMode == NoMotion && ! m_vData.dragging) setAxisMoving(VerticalAxis, false);
    if (m_hData.motionMode == NoMotion) setAxisFlicking(HorizontalAxis, false);
    if (m_vData.motionMode == NoMotion) setAxisFlicking(VerticalAxis, false);

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

auto Flickable::draggingEnding() -> void {
    const bool wasDragging = isDragging();
    setAxisDragging(HorizontalAxis, false);
    setAxisDragging(VerticalAxis, false);
    if (wasDragging) emit dragEnded();
}

auto Flickable::startAxisFlick(Axis axis, qreal velocity) -> void {
    if (qFuzzyIsNull(velocity)) {
        fixup(axis);
        return;
    }

    auto& data         = axisData(axis);
    velocity           = std::clamp(velocity, -m_maxVelocity, m_maxVelocity);
    const qreal decel  = std::max<qreal>(1, m_deceleration);
    const qreal dist   = (velocity * velocity) / (2.0 * decel);
    const qreal target = boundedPosition(axis, axisPosition(axis) + (velocity > 0 ? dist : -dist));

    data.motionMode     = FlickMotion;
    data.motionStart    = axisPosition(axis);
    data.motionTarget   = target;
    data.motionVelocity = velocity;
    data.motionDuration = std::abs(velocity) / decel;
    data.motionElapsed  = 0;
    setAxisVelocity(axis, velocity);
    setAxisMoving(axis, true);
}

auto Flickable::stopAxisMotion(Axis axis) -> void {
    auto& data          = axisData(axis);
    data.motionMode     = NoMotion;
    data.motionVelocity = 0;
    data.motionElapsed  = 0;
    setAxisVelocity(axis, 0);
    setAxisFlicking(axis, false);
}

auto Flickable::ensureMotionTimer() -> void {
    if (! m_motionTimer.isActive()) {
        m_motionClock.restart();
        m_motionTimer.start(16, this);
    }
}

auto Flickable::advanceAxis(Axis axis, qreal deltaSeconds) -> void {
    auto& data = axisData(axis);
    if (data.motionMode == NoMotion) return;

    data.motionElapsed += deltaSeconds;

    if (data.motionMode == FlickMotion) {
        qreal       velocity = data.motionVelocity;
        qreal       position = axisPosition(axis) + velocity * deltaSeconds;
        const qreal decel    = std::max<qreal>(1, m_deceleration);
        if (velocity > 0)
            velocity = std::max<qreal>(0, velocity - decel * deltaSeconds);
        else
            velocity = std::min<qreal>(0, velocity + decel * deltaSeconds);

        position = boundedPosition(axis, position);
        if (qFuzzyCompare(position, minExtent(axis)) ||
            qFuzzyCompare(position, maxExtent(axis)))
            velocity = 0;

        setAxisPosition(axis, position);
        setAxisVelocity(axis, velocity);
        data.motionVelocity = velocity;

        if (qFuzzyIsNull(velocity) || data.motionElapsed >= data.motionDuration) {
            stopAxisMotion(axis);
            fixup(axis);
        }
        return;
    }
}

auto Flickable::fixup(Axis axis) -> void {
    setAxisPosition(axis, axisPosition(axis));
}

auto Flickable::isAxisAnimating(Axis axis) const -> bool {
    return axisData(axis).motionMode != NoMotion;
}

auto Flickable::handlePress(const QPointF& position, qint64 timestamp) -> void {
    cancelFlick();
    m_pressed       = true;
    m_stealMouse    = false;
    m_pressPos      = position;
    m_lastPos       = position;
    m_lastPressTime = timestamp;
    m_lastPosTime   = timestamp;

    auto setupAxis = [this, timestamp, position](Axis axis) -> void {
        auto& data = axisData(axis);
        data.resetDrag();
        data.pressPos        = axis == HorizontalAxis ? position.x() : position.y();
        data.lastPos         = data.pressPos;
        data.pressContentPos = axisPosition(axis);
        data.addVelocitySample(timestamp, data.pressPos, m_maxVelocity);
    };
    setupAxis(HorizontalAxis);
    setupAxis(VerticalAxis);

    if (m_pressDelay > 0) m_pressDelayTimer.start(m_pressDelay, this);
}

auto Flickable::handleMove(const QPointF& position, qint64 timestamp, Qt::MouseButtons buttons)
    -> void {
    if (! m_pressed || buttons == Qt::NoButton) return;

    const QPointF delta     = position - m_pressPos;
    const int     threshold = qGuiApp->styleHints()->startDragDistance();
    bool          stealX    = false;
    bool          stealY    = false;

    auto handleAxis = [&](Axis axis, qreal deltaValue, qreal currentPos) -> bool {
        if (! axisCanFlick(axis)) return false;
        auto&      data          = axisData(axis);
        const bool overThreshold = std::abs(deltaValue) >= threshold || isMoving();
        if (! overThreshold) {
            data.addVelocitySample(timestamp, currentPos, m_maxVelocity);
            return false;
        }

        if (! data.dragging) data.dragStartOffset = m_syncDrag ? 0 : deltaValue;
        const qreal rawPosition  = data.pressContentPos - (deltaValue - data.dragStartOffset);
        const qreal nextPosition = boundedPosition(axis, rawPosition);
        setAxisPosition(axis, nextPosition);
        data.previousDragDelta = deltaValue;
        data.addVelocitySample(timestamp, currentPos, m_maxVelocity);
        return true;
    };

    stealX = handleAxis(HorizontalAxis, delta.x(), position.x());
    stealY = handleAxis(VerticalAxis, delta.y(), position.y());

    if (stealX || stealY) {
        m_stealMouse = true;
        setKeepMouseGrab(true);
        m_pressDelayTimer.stop();
        draggingStarting(stealX, stealY);
        movementStarting();
    }

    m_lastPos     = position;
    m_lastPosTime = timestamp;
}

auto Flickable::handleRelease(const QPointF& position, qint64 timestamp) -> void {
    if (! m_pressed) return;
    Q_UNUSED(position)
    m_pressed    = false;
    m_stealMouse = false;
    setKeepMouseGrab(false);
    m_pressDelayTimer.stop();

    m_hData.updateVelocity();
    m_vData.updateVelocity();

    const qint64 elapsed     = m_lastPosTime < 0 ? 1000 : timestamp - m_lastPosTime;
    const bool   wasDragging = isDragging();
    draggingEnding();

    bool          flickedX       = false;
    bool          flickedY       = false;
    const int     flickThreshold = qGuiApp->styleHints()->startDragDistance();
    const QPointF totalDelta     = m_lastPos - m_pressPos;

    if (elapsed < 100 && wasDragging) {
        if (xflick() && std::abs(m_hData.velocity) > MinimumFlickVelocity &&
            std::abs(totalDelta.x()) > flickThreshold) {
            startAxisFlick(HorizontalAxis, -m_hData.velocity);
            flickedX = true;
        } else {
            fixup(HorizontalAxis);
        }
        if (yflick() && std::abs(m_vData.velocity) > MinimumFlickVelocity &&
            std::abs(totalDelta.y()) > flickThreshold) {
            startAxisFlick(VerticalAxis, -m_vData.velocity);
            flickedY = true;
        } else {
            fixup(VerticalAxis);
        }
    } else {
        fixup(HorizontalAxis);
        fixup(VerticalAxis);
    }

    flickingStarted(flickedX, flickedY);
    if (isAxisAnimating(HorizontalAxis) || isAxisAnimating(VerticalAxis)) {
        movementStarting();
        ensureMotionTimer();
    } else {
        movementEnding();
    }
}

auto Flickable::cancelInteraction() -> void {
    if (! m_pressed && ! isDragging()) return;
    m_pressed    = false;
    m_stealMouse = false;
    setKeepMouseGrab(false);
    draggingEnding();
    fixup(HorizontalAxis);
    fixup(VerticalAxis);
    if (! isAxisAnimating(HorizontalAxis) && ! isAxisAnimating(VerticalAxis)) movementEnding();
}

auto Flickable::buttonsAccepted(const QSinglePointEvent* event) const -> bool {
    if (! event) return false;
    return ((event->button() & m_acceptedButtons) != 0) ||
           ((event->buttons() & m_acceptedButtons) != 0);
}

auto Flickable::pointerAccepted(QQuickItem* receiver, QEvent* event) const -> bool {
    if (! event) return false;
    if (event->type() == QEvent::UngrabMouse) return true;
    return acceptsPoint(eventPosition(receiver, event));
}

auto Flickable::acceptsPoint(const QPointF& point) const -> bool {
    if (! contains(point)) return false;
    if (m_inputMaskMode == Viewport) return true;

    auto* item = m_inputMaskMode == CustomItem ? interactionItem() : m_contentItem;
    if (! item || ! item->isVisible() || ! item->isEnabled()) return false;
    return item->contains(item->mapFromItem(this, point));
}

auto Flickable::eventPosition(QQuickItem* receiver, QEvent* event) const -> QPointF {
    if (auto* mouse = dynamic_cast<QMouseEvent*>(event))
        return receiver ? receiver->mapToItem(const_cast<Flickable*>(this), mouse->position())
                        : mouse->position();
    if (auto* wheel = dynamic_cast<QWheelEvent*>(event))
        return receiver ? receiver->mapToItem(const_cast<Flickable*>(this), wheel->position())
                        : wheel->position();
    if (auto* pointer = dynamic_cast<QPointerEvent*>(event)) {
        if (pointer->points().isEmpty()) return {};
        return mapFromScene(pointer->points().first().scenePosition());
    }
    return {};
}

auto Flickable::ignorePointerEvent(QEvent* event) const -> void {
    if (auto* pointer = dynamic_cast<QPointerEvent*>(event))
        pointer->setAccepted(false);
    else
        event->ignore();
}

auto Flickable::childMouseEventFilter(QQuickItem* item, QEvent* event) -> bool {
    if (! m_interactive || ! item || ! isVisible() || ! isEnabled() ||
        ! pointerAccepted(item, event)) {
        cancelInteraction();
        return QQuickItem::childMouseEventFilter(item, event);
    }

    if (event->type() == QEvent::UngrabMouse) {
        mouseUngrabEvent();
        return QQuickItem::childMouseEventFilter(item, event);
    }

    if (! event->isPointerEvent()) return QQuickItem::childMouseEventFilter(item, event);

    auto* pointer = static_cast<QPointerEvent*>(event);
    if (pointer->points().isEmpty() || pointer->pointCount() > 1) return false;

    const QPointF local  = eventPosition(item, event);
    const auto    state  = pointer->points().first().state();
    bool          filter = m_stealMouse || isMoving();

    if (state == QEventPoint::State::Pressed) {
        if (auto* single = dynamic_cast<QSinglePointEvent*>(event);
            single && ! buttonsAccepted(single))
            return QQuickItem::childMouseEventFilter(item, event);
        handlePress(local, eventTimestamp(pointer));
        filter = false;
    } else if (state == QEventPoint::State::Updated) {
        handleMove(local, eventTimestamp(pointer), Qt::LeftButton);
        filter = m_stealMouse || isMoving();
        if (filter) pointer->setExclusiveGrabber(pointer->points().first(), this);
    } else if (state == QEventPoint::State::Released) {
        const bool wasStealing = m_stealMouse || isMoving();
        handleRelease(local, eventTimestamp(pointer));
        filter = wasStealing;
    }

    if (filter) {
        event->accept();
        return true;
    }
    return QQuickItem::childMouseEventFilter(item, event);
}

auto Flickable::mousePressEvent(QMouseEvent* event) -> void {
    if (m_interactive && buttonsAccepted(event) && acceptsPoint(event->position())) {
        handlePress(event->position(), eventTimestamp(event));
        event->accept();
        return;
    }
    QQuickItem::mousePressEvent(event);
}

auto Flickable::mouseMoveEvent(QMouseEvent* event) -> void {
    if (m_interactive && buttonsAccepted(event) && acceptsPoint(event->position())) {
        handleMove(event->position(), eventTimestamp(event), event->buttons());
        event->accept();
        return;
    }
    QQuickItem::mouseMoveEvent(event);
}

auto Flickable::mouseReleaseEvent(QMouseEvent* event) -> void {
    if (m_interactive && buttonsAccepted(event)) {
        handleRelease(event->position(), eventTimestamp(event));
        event->accept();
        return;
    }
    QQuickItem::mouseReleaseEvent(event);
}

auto Flickable::touchEvent(QTouchEvent* event) -> void {
    if (! m_interactive || ! acceptsPoint(eventPosition(nullptr, event)) ||
        event->pointCount() != 1) {
        QQuickItem::touchEvent(event);
        return;
    }

    if (event->type() == QEvent::TouchCancel) {
        cancelInteraction();
        event->accept();
        return;
    }

    const auto&   point = event->points().first();
    const QPointF local = mapFromScene(point.scenePosition());
    switch (point.state()) {
    case QEventPoint::State::Pressed:
        handlePress(local, eventTimestamp(event));
        event->accept();
        break;
    case QEventPoint::State::Updated:
        handleMove(local, eventTimestamp(event), Qt::LeftButton);
        event->accept();
        break;
    case QEventPoint::State::Released:
        handleRelease(local, eventTimestamp(event));
        event->accept();
        break;
    default: QQuickItem::touchEvent(event); break;
    }
}

auto Flickable::wheelEvent(QWheelEvent* event) -> void {
    if (! m_interactive || ! acceptsPoint(event->position())) {
        QQuickItem::wheelEvent(event);
        return;
    }

    QPointF delta = event->pixelDelta();
    if (delta.isNull()) {
        const qreal step = 24.0 * qGuiApp->styleHints()->wheelScrollLines();
        delta            = QPointF(event->angleDelta()) / 120.0 * step;
    }
    if (event->inverted()) delta = -delta;

    bool moved = false;
    if (xflick() && ! qFuzzyIsNull(delta.x())) {
        const qreal old = contentX();
        setAxisPosition(HorizontalAxis, boundedPosition(HorizontalAxis, old - delta.x()));
        moved = moved || ! qFuzzyCompare(old, contentX());
    }
    if (yflick() && ! qFuzzyIsNull(delta.y())) {
        const qreal old = contentY();
        setAxisPosition(VerticalAxis, boundedPosition(VerticalAxis, old - delta.y()));
        moved = moved || ! qFuzzyCompare(old, contentY());
    }

    if (moved) {
        movementStarting();
        movementEnding();
        event->accept();
        return;
    }
    QQuickItem::wheelEvent(event);
}

auto Flickable::timerEvent(QTimerEvent* event) -> void {
    if (event->timerId() == m_motionTimer.timerId()) {
        const qreal dt = std::min<qreal>(0.05, qreal(m_motionClock.restart()) / 1000.0);
        advanceAxis(HorizontalAxis, dt);
        advanceAxis(VerticalAxis, dt);
        if (! isAxisAnimating(HorizontalAxis) && ! isAxisAnimating(VerticalAxis)) {
            m_motionTimer.stop();
            movementEnding();
        }
        event->accept();
        return;
    }
    if (event->timerId() == m_pressDelayTimer.timerId()) {
        m_pressDelayTimer.stop();
        event->accept();
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
    if (! m_pressed) {
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

auto Flickable::mouseUngrabEvent() -> void { cancelInteraction(); }

auto Flickable::minXExtent() const -> qreal { return originX() - m_hData.startMargin; }
auto Flickable::minYExtent() const -> qreal { return originY() - m_vData.startMargin; }

auto Flickable::maxXExtent() const -> qreal {
    return std::max(minXExtent(), vWidth() + m_hData.endMargin + originX() - width());
}

auto Flickable::maxYExtent() const -> qreal {
    return std::max(minYExtent(), vHeight() + m_vData.endMargin + originY() - height());
}

auto Flickable::vWidth() const -> qreal {
    return m_hData.viewSize < 0 ? m_contentItem->width() : m_hData.viewSize;
}

auto Flickable::vHeight() const -> qreal {
    return m_vData.viewSize < 0 ? m_contentItem->height() : m_vData.viewSize;
}

auto Flickable::viewportMoved(Qt::Orientations orientation) -> void { Q_UNUSED(orientation) }

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
