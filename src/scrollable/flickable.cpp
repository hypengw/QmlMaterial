#include "qml_material/scrollable/flickable.hpp"
#include "pointer_delivery.hpp"

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

auto Flickable::setContentY(qreal position) -> void {
    setContentPosition(VerticalAxis, position);
}

void Flickable::setContentPosition(Axis axis, qreal position) {
    if (! std::isfinite(position)) return;
    QPointer<Flickable> guard(this);
    cancelInteraction();
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
    for (Axis axis : { HorizontalAxis, VerticalAxis }) {
        auto&       data       = axisData(axis);
        const qreal correction = axis == HorizontalAxis ? anchorDelta.x() : anchorDelta.y();
        const qreal desired    = data.position + correction;
        const qreal position   = boundedPosition(axis, aligned(desired));
        const qreal applied    = position - data.position;
        data.motion.translate(applied);
        data.pressContentPos += applied;
        if (position != data.position) m_geometryNotifications |= axis == HorizontalAxis ? 4 : 8;
        data.position = position;
        if (desired < minExtent(axis) || desired > maxExtent(axis)) data.motion.stop();
    }
    ++m_geometryRevision;
    flushContentGeometry();
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
    cancelInteraction();
    if (! guard) return;
    cancelFlick();
    if (! guard) return;
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
        requestMotionFrame();
    }
}

void Flickable::cancelFlick() {
    QPointer<Flickable> guard(this);
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
    const bool replacing = m_motionWindowAttached;
    m_motionWindowAttached = target != nullptr;
    disconnect(m_frameConnection);
    disconnect(m_windowVisibilityConnection);
    m_frameConnection = {};
    m_windowVisibilityConnection = {};
    if (target) {
        m_frameConnection =
            connect(target, &QQuickWindow::afterAnimating, this, &Flickable::requestMotionFrame);
        m_windowVisibilityConnection = connect(target, &QWindow::visibleChanged, this,
                                               [this](bool visible) {
            if (! visible) cancelScrollActivity();
        });
    }
    if (replacing) cancelScrollActivity();
}

void Flickable::cancelScrollActivity() {
    QPointer<Flickable> guard(this);
    cancelInteraction();
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

auto Flickable::handlePress(const QPointF& position, qint64 timestamp) -> void {
    QPointer<Flickable> guard(this);
    scrollInputStarted();
    if (! guard) return;
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
        clearDelayedPress();
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
    setKeepTouchGrab(false);
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
        requestMotionFrame();
    } else {
        movementEnding();
    }
}

auto Flickable::cancelInteraction() -> void {
    clearDelayedPress();
    if (! m_pressed && ! isDragging()) return;
    m_pressed    = false;
    m_stealMouse = false;
    m_pressDelayTimer.stop();
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    draggingEnding();
    fixup(HorizontalAxis);
    fixup(VerticalAxis);
    if (! isAxisAnimating(HorizontalAxis) && ! isAxisAnimating(VerticalAxis)) movementEnding();
}

bool Flickable::captureDelayedPress(QQuickItem* receiver, QPointerEvent* event) {
    if (m_pressDelay <= 0 || ! window()) return false;
    for (auto* item = receiver; item; item = item->parentItem()) {
        auto* flick = qobject_cast<Flickable*>(item);
        if (flick && flick->isInteractive() && flick->pressDelay() > 0) {
            if (flick != this) return false;
            break;
        }
    }
    m_delayedPress = pointer_delivery::cloneForWindow(event);
    m_delayedPress->setAccepted(false);
    m_delayedReceiver = receiver;
    m_delayedWindow = window();
    m_pressDelayTimer.start(m_pressDelay, this);
    setKeepMouseGrab(true);
    setKeepTouchGrab(true);
    event->setExclusiveGrabber(event->points().first(), this);
    return true;
}

void Flickable::clearDelayedPress() {
    if (m_delayedPress) {
        setKeepMouseGrab(m_stealMouse);
        setKeepTouchGrab(m_stealMouse);
    }
    m_pressDelayTimer.stop();
    m_delayedPress.reset();
    m_delayedReceiver.clear();
    m_delayedWindow.clear();
}

void Flickable::replayDelayedPress(QPointerEvent* release) {
    if (! m_delayedPress) return;
    auto event = std::move(m_delayedPress);
    QPointer<QQuickWindow> target = m_delayedWindow;
    const bool valid = target && target == window() && m_delayedReceiver &&
        m_contentItem->isAncestorOf(m_delayedReceiver) &&
        m_delayedReceiver->isVisible() && m_delayedReceiver->isEnabled() && m_interactive;
    clearDelayedPress();
    if (! valid) {
        cancelInteraction();
        return;
    }
    QPointer<Flickable> guard(this);
    auto released = release ? pointer_delivery::cloneForWindow(release) : nullptr;
    m_replayingPress = true;
    setKeepMouseGrab(false);
    setKeepTouchGrab(false);
    const auto point = event->points().first();
    if (event->exclusiveGrabber(point) == this) event->setExclusiveGrabber(point, nullptr);
    if (! guard) return;
    if (! target) {
        m_replayingPress = false;
        cancelInteraction();
        return;
    }
    pointer_delivery::send(this, target, event.get());
    if (! guard) return;
    if (released && target && target == window())
        pointer_delivery::send(this, target, released.get());
    if (! guard) return;
    m_replayingPress = false;
    if (release) cancelInteraction();
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
        const auto* pointer = dynamic_cast<QPointerEvent*>(event);
        // Losing a child's grab to this view is the start of dragging, not cancellation.
        if (! pointer || pointer->points().isEmpty() ||
            pointer->exclusiveGrabber(pointer->points().first()) != this)
            mouseUngrabEvent();
        return QQuickItem::childMouseEventFilter(item, event);
    }

    if (! event->isPointerEvent()) return QQuickItem::childMouseEventFilter(item, event);

    auto* pointer = static_cast<QPointerEvent*>(event);
    if (pointer->points().isEmpty() || pointer->pointCount() > 1) return false;
    if (pointer->exclusiveGrabber(pointer->points().first()) == this) return false;
    if (item->keepMouseGrab() || item->keepTouchGrab()) {
        cancelInteraction();
        return false;
    }

    const QPointF local  = eventPosition(item, event);
    const auto    state  = pointer->points().first().state();
    bool          filter = m_stealMouse || isMoving();

    if (state == QEventPoint::State::Pressed) {
        if (auto* single = dynamic_cast<QSinglePointEvent*>(event);
            single && ! buttonsAccepted(single))
            return QQuickItem::childMouseEventFilter(item, event);
        handlePress(local, eventTimestamp(pointer));
        filter = captureDelayedPress(item, pointer);
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
    if (m_replayingPress) { event->ignore(); return; }
    if (m_interactive && buttonsAccepted(event) && acceptsPoint(event->position())) {
        handlePress(event->position(), eventTimestamp(event));
        event->accept();
        return;
    }
    QQuickItem::mousePressEvent(event);
}

auto Flickable::mouseMoveEvent(QMouseEvent* event) -> void {
    if (m_interactive && buttonsAccepted(event) &&
        (m_pressed || acceptsPoint(event->position()))) {
        handleMove(event->position(), eventTimestamp(event), event->buttons());
        event->accept();
        return;
    }
    QQuickItem::mouseMoveEvent(event);
}

auto Flickable::mouseReleaseEvent(QMouseEvent* event) -> void {
    if (m_delayedPress) {
        replayDelayedPress(event);
        event->accept();
        return;
    }
    if (m_interactive && buttonsAccepted(event)) {
        handleRelease(event->position(), eventTimestamp(event));
        event->accept();
        return;
    }
    QQuickItem::mouseReleaseEvent(event);
}

auto Flickable::touchEvent(QTouchEvent* event) -> void {
    if (m_replayingPress) { event->ignore(); return; }
    if (event->type() == QEvent::TouchCancel) {
        cancelInteraction();
        event->accept();
        return;
    }

    if (! m_interactive || event->pointCount() != 1 ||
        (! m_pressed && ! acceptsPoint(eventPosition(nullptr, event)))) {
        QQuickItem::touchEvent(event);
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
        if (m_delayedPress) {
            replayDelayedPress(event);
            event->accept();
            return;
        }
        handleRelease(local, eventTimestamp(event));
        event->accept();
        break;
    default: QQuickItem::touchEvent(event); break;
    }
}

auto Flickable::consumeScroll(QPointF delta, ScrollInput input, Qt::ScrollPhase phase)
    -> ScrollConsumption {
    ScrollConsumption result { {}, delta };
    if (! isInteractive() || ! isEnabled() || ! isVisible()) return result;
    if (! std::isfinite(delta.x()) || ! std::isfinite(delta.y())) return result;
    QPointer<Flickable> guard(this);
    if (! delta.isNull() || phase == Qt::ScrollBegin) {
        scrollInputStarted();
        if (! guard) return result;
        cancelInteraction();
    }
    if (! guard) return result;
    if (phase == Qt::ScrollBegin) cancelFlick();
    if (! guard) return result;
    const qreal now = m_motionClock.nsecsElapsed() / 1e9;
    for (Axis axis : { HorizontalAxis, VerticalAxis }) {
        auto& data = axisData(axis);
        if (phase == Qt::ScrollEnd) data.platformScrolling = false;
        const qreal requested = axis == HorizontalAxis ? delta.x() : delta.y();
        if (! axisCanFlick(axis) || qFuzzyIsNull(requested)) continue;
        const bool  smooth   = input == ScrollInput::Smooth && window();
        const bool  retarget = smooth && data.motion.mode() == ScrollMotion::Mode::Target;
        const qreal current  = axisPosition(axis);
        const qreal base     = retarget ? data.motion.target() : current;
        const qreal target   = boundedPosition(axis, base + requested);
        qreal       consumed = target - base;
        if (smooth) {
            if (qFuzzyIsNull(consumed)) continue;
            const auto  sample     = data.motion.sample(now);
            const qreal velocity   = data.motion.active() ? sample.velocity : 0;
            data.platformScrolling = false;
            data.motion.smooth(current, velocity, target, now);
            const bool wasFlicking = isFlicking();
            setAxisFlicking(axis, false);
            if (wasFlicking && ! isFlicking()) emit flickEnded();
            movementStarting();
        } else {
            stopAxisMotion(axis);
            if (! qFuzzyIsNull(consumed)) {
                data.platformScrolling = true;
                movementStarting();
                setAxisPosition(axis, target);
                consumed = axisPosition(axis) - current;
            }
            if (phase == Qt::NoScrollPhase || phase == Qt::ScrollEnd)
                data.platformScrolling = false;
        }
        if (axis == HorizontalAxis) {
            result.consumed.setX(consumed);
            result.remaining.setX(requested - consumed);
        } else {
            result.consumed.setY(consumed);
            result.remaining.setY(requested - consumed);
        }
    }
    movementEnding();
    requestMotionFrame();
    return result;
}

auto Flickable::wheelEvent(QWheelEvent* event) -> void {
    if (! m_interactive || (event->phase() != Qt::ScrollEnd && ! acceptsPoint(event->position()))) {
        QQuickItem::wheelEvent(event);
        return;
    }

    QPointF delta = event->pixelDelta();
    if (delta.isNull()) {
        const qreal step = 24.0 * qGuiApp->styleHints()->wheelScrollLines();
        delta            = QPointF(event->angleDelta()) / 120.0 * step;
    }
    // Qt deltas already carry the platform's natural-scrolling direction.
    const ScrollInput   input = event->pixelDelta().isNull() && event->phase() == Qt::NoScrollPhase
                                    ? ScrollInput::Smooth
                                    : ScrollInput::Direct;
    QPointF             remaining = -delta;
    bool                consumed  = false;
    QPointer<Flickable> owner     = this;
    QPointF             point     = event->position();
    while (owner) {
        QPointer<Flickable> next;
        for (auto* parent = owner->parentItem(); parent; parent = parent->parentItem()) {
            if (auto* candidate = qobject_cast<Flickable*>(parent)) {
                next = candidate;
                break;
            }
        }
        const QPointF mappedPoint = next ? owner->mapToItem(next, point) : QPointF();
        const QPointF basisX =
            next ? owner->mapToItem(next, QPointF(1, 0)) - owner->mapToItem(next, QPointF())
                 : QPointF();
        const QPointF basisY =
            next ? owner->mapToItem(next, QPointF(0, 1)) - owner->mapToItem(next, QPointF())
                 : QPointF();
        if (event->phase() == Qt::ScrollEnd || owner->acceptsPoint(point)) {
            const auto result = owner->consumeScroll(remaining, input, event->phase());
            consumed |= ! result.consumed.isNull();
            remaining = result.remaining;
        }
        remaining = basisX * remaining.x() + basisY * remaining.y();
        point     = mappedPoint;
        owner     = next;
    }
    event->setAccepted(consumed);
}

auto Flickable::timerEvent(QTimerEvent* event) -> void {
    if (event->timerId() == m_pressDelayTimer.timerId()) {
        replayDelayedPress();
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

auto Flickable::mouseUngrabEvent() -> void { if (! m_replayingPress) cancelInteraction(); }
auto Flickable::touchUngrabEvent() -> void { if (! m_replayingPress) cancelInteraction(); }

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
void Flickable::setVerticalScrollTarget(qreal position) {
    if (! std::isfinite(position)) return;
    position     = boundedPosition(VerticalAxis, position);
    auto& motion = m_vData.motion;
    if (motion.mode() == ScrollMotion::Mode::Target && motion.target() == position) return;
    if (! window() || (qAbs(position - contentY()) < 0.01 && qAbs(verticalVelocity()) < 0.1)) {
        stopAxisMotion(VerticalAxis);
        setAxisPosition(VerticalAxis, position);
        movementEnding();
        return;
    }
    const qreal now      = m_motionClock.nsecsElapsed() / 1e9;
    const qreal velocity = motion.active() ? motion.sample(now).velocity : 0;
    motion.smooth(contentY(), velocity, position, now);
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
