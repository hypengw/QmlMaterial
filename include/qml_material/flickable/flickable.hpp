#pragma once

#include <QBasicTimer>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QQuickItem>
#include <QVector>

namespace qml_material
{

class Flickable;

class FlickableVisibleArea : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qreal xPosition READ xPosition NOTIFY xPositionChanged FINAL)
    Q_PROPERTY(qreal yPosition READ yPosition NOTIFY yPositionChanged FINAL)
    Q_PROPERTY(qreal widthRatio READ widthRatio NOTIFY widthRatioChanged FINAL)
    Q_PROPERTY(qreal heightRatio READ heightRatio NOTIFY heightRatioChanged FINAL)

public:
    explicit FlickableVisibleArea(Flickable* parent = nullptr);

    auto xPosition() const -> qreal;
    auto yPosition() const -> qreal;
    auto widthRatio() const -> qreal;
    auto heightRatio() const -> qreal;

    auto updateVisible() -> void;

    Q_SIGNAL void xPositionChanged(qreal value);
    Q_SIGNAL void yPositionChanged(qreal value);
    Q_SIGNAL void widthRatioChanged(qreal value);
    Q_SIGNAL void heightRatioChanged(qreal value);

private:
    QPointer<Flickable> m_flickable;
    qreal               m_xPosition { 0 };
    qreal               m_yPosition { 0 };
    qreal               m_widthRatio { 1 };
    qreal               m_heightRatio { 1 };
};

class Flickable : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(Flickable2)

    Q_PROPERTY(
        qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(
        qreal contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(qreal contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(qreal contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT)

    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin NOTIFY topMarginChanged)
    Q_PROPERTY(
        qreal bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY bottomMarginChanged)
    Q_PROPERTY(qreal originY READ originY NOTIFY originYChanged)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged)
    Q_PROPERTY(qreal originX READ originX NOTIFY originXChanged)

    Q_PROPERTY(qreal horizontalVelocity READ horizontalVelocity NOTIFY horizontalVelocityChanged)
    Q_PROPERTY(qreal verticalVelocity READ verticalVelocity NOTIFY verticalVelocityChanged)
    Q_PROPERTY(qreal maximumFlickVelocity READ maximumFlickVelocity WRITE setMaximumFlickVelocity
                   NOTIFY maximumFlickVelocityChanged)
    Q_PROPERTY(qreal flickDeceleration READ flickDeceleration WRITE setFlickDeceleration NOTIFY
                   flickDecelerationChanged)
    Q_PROPERTY(bool moving READ isMoving NOTIFY movingChanged)
    Q_PROPERTY(bool movingHorizontally READ isMovingHorizontally NOTIFY movingHorizontallyChanged)
    Q_PROPERTY(bool movingVertically READ isMovingVertically NOTIFY movingVerticallyChanged)
    Q_PROPERTY(bool flicking READ isFlicking NOTIFY flickingChanged)
    Q_PROPERTY(
        bool flickingHorizontally READ isFlickingHorizontally NOTIFY flickingHorizontallyChanged)
    Q_PROPERTY(bool flickingVertically READ isFlickingVertically NOTIFY flickingVerticallyChanged)
    Q_PROPERTY(bool dragging READ isDragging NOTIFY draggingChanged)
    Q_PROPERTY(
        bool draggingHorizontally READ isDraggingHorizontally NOTIFY draggingHorizontallyChanged)
    Q_PROPERTY(bool draggingVertically READ isDraggingVertically NOTIFY draggingVerticallyChanged)
    Q_PROPERTY(FlickableDirection flickableDirection READ flickableDirection WRITE
                   setFlickableDirection NOTIFY flickableDirectionChanged)

    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(int pressDelay READ pressDelay WRITE setPressDelay NOTIFY pressDelayChanged)
    Q_PROPERTY(bool atXEnd READ isAtXEnd NOTIFY atXEndChanged)
    Q_PROPERTY(bool atYEnd READ isAtYEnd NOTIFY atYEndChanged)
    Q_PROPERTY(bool atXBeginning READ isAtXBeginning NOTIFY atXBeginningChanged)
    Q_PROPERTY(bool atYBeginning READ isAtYBeginning NOTIFY atYBeginningChanged)
    Q_PROPERTY(FlickableVisibleArea* visibleArea READ visibleArea CONSTANT)
    Q_PROPERTY(bool pixelAligned READ pixelAligned WRITE setPixelAligned NOTIFY pixelAlignedChanged)
    Q_PROPERTY(bool synchronousDrag READ synchronousDrag WRITE setSynchronousDrag NOTIFY
                   synchronousDragChanged)
    Q_PROPERTY(Qt::MouseButtons acceptedButtons READ acceptedButtons WRITE setAcceptedButtons NOTIFY
                   acceptedButtonsChanged FINAL)
    Q_PROPERTY(InputMaskMode inputMaskMode READ inputMaskMode WRITE setInputMaskMode NOTIFY
                   inputMaskModeChanged FINAL)
    Q_PROPERTY(QQuickItem* interactionItem READ interactionItem WRITE setInteractionItem RESET
                   resetInteractionItem NOTIFY interactionItemChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> flickableData READ flickableData)
    Q_PROPERTY(QQmlListProperty<QQuickItem> flickableChildren READ flickableChildren)
    Q_CLASSINFO("DefaultProperty", "flickableData")

public:
    explicit Flickable(QQuickItem* parent = nullptr);
    ~Flickable() override;

    auto flickableData() -> QQmlListProperty<QObject>;
    auto flickableChildren() -> QQmlListProperty<QQuickItem>;

    enum FlickableDirection
    {
        AutoFlickDirection         = 0x0,
        HorizontalFlick            = 0x1,
        VerticalFlick              = 0x2,
        HorizontalAndVerticalFlick = 0x3,
        AutoFlickIfNeeded          = 0xc,
    };
    Q_ENUM(FlickableDirection)

    enum InputMaskMode
    {
        Viewport,
        ContentOnly,
        CustomItem,
    };
    Q_ENUM(InputMaskMode)

    auto contentWidth() const -> qreal;
    auto setContentWidth(qreal value) -> void;

    auto contentHeight() const -> qreal;
    auto setContentHeight(qreal value) -> void;

    auto         contentX() const -> qreal;
    virtual auto setContentX(qreal position) -> void;

    auto         contentY() const -> qreal;
    virtual auto setContentY(qreal position) -> void;

    auto topMargin() const -> qreal;
    auto setTopMargin(qreal value) -> void;

    auto bottomMargin() const -> qreal;
    auto setBottomMargin(qreal value) -> void;

    auto leftMargin() const -> qreal;
    auto setLeftMargin(qreal value) -> void;

    auto rightMargin() const -> qreal;
    auto setRightMargin(qreal value) -> void;

    virtual auto originY() const -> qreal;
    virtual auto originX() const -> qreal;

    auto isMoving() const -> bool;
    auto isMovingHorizontally() const -> bool;
    auto isMovingVertically() const -> bool;
    auto isFlicking() const -> bool;
    auto isFlickingHorizontally() const -> bool;
    auto isFlickingVertically() const -> bool;
    auto isDragging() const -> bool;
    auto isDraggingHorizontally() const -> bool;
    auto isDraggingVertically() const -> bool;

    auto pressDelay() const -> int;
    auto setPressDelay(int delay) -> void;

    auto maximumFlickVelocity() const -> qreal;
    auto setMaximumFlickVelocity(qreal value) -> void;

    auto flickDeceleration() const -> qreal;
    auto setFlickDeceleration(qreal value) -> void;

    auto isInteractive() const -> bool;
    auto setInteractive(bool value) -> void;

    auto horizontalVelocity() const -> qreal;
    auto verticalVelocity() const -> qreal;

    auto isAtXEnd() const -> bool;
    auto isAtXBeginning() const -> bool;
    auto isAtYEnd() const -> bool;
    auto isAtYBeginning() const -> bool;

    auto contentItem() const -> QQuickItem*;

    auto flickableDirection() const -> FlickableDirection;
    auto setFlickableDirection(FlickableDirection direction) -> void;

    auto pixelAligned() const -> bool;
    auto setPixelAligned(bool align) -> void;

    auto synchronousDrag() const -> bool;
    auto setSynchronousDrag(bool value) -> void;

    auto acceptedButtons() const -> Qt::MouseButtons;
    auto setAcceptedButtons(Qt::MouseButtons buttons) -> void;

    auto inputMaskMode() const -> InputMaskMode;
    auto setInputMaskMode(InputMaskMode mode) -> void;

    auto interactionItem() const -> QQuickItem*;
    auto setInteractionItem(QQuickItem* item) -> void;
    auto resetInteractionItem() -> void;

    auto visibleArea() -> FlickableVisibleArea*;

    Q_INVOKABLE auto resizeContent(qreal width, qreal height, QPointF center) -> void;
    Q_INVOKABLE auto flick(qreal xVelocity, qreal yVelocity) -> void;
    Q_INVOKABLE auto cancelFlick() -> void;

    Q_SIGNAL void contentWidthChanged();
    Q_SIGNAL void contentHeightChanged();
    Q_SIGNAL void contentXChanged();
    Q_SIGNAL void contentYChanged();
    Q_SIGNAL void topMarginChanged();
    Q_SIGNAL void bottomMarginChanged();
    Q_SIGNAL void leftMarginChanged();
    Q_SIGNAL void rightMarginChanged();
    Q_SIGNAL void originYChanged();
    Q_SIGNAL void originXChanged();
    Q_SIGNAL void movingChanged();
    Q_SIGNAL void movingHorizontallyChanged();
    Q_SIGNAL void movingVerticallyChanged();
    Q_SIGNAL void flickingChanged();
    Q_SIGNAL void flickingHorizontallyChanged();
    Q_SIGNAL void flickingVerticallyChanged();
    Q_SIGNAL void draggingChanged();
    Q_SIGNAL void draggingHorizontallyChanged();
    Q_SIGNAL void draggingVerticallyChanged();
    Q_SIGNAL void horizontalVelocityChanged();
    Q_SIGNAL void verticalVelocityChanged();
    Q_SIGNAL void isAtBoundaryChanged();
    Q_SIGNAL void flickableDirectionChanged();
    Q_SIGNAL void interactiveChanged();
    Q_SIGNAL void maximumFlickVelocityChanged();
    Q_SIGNAL void flickDecelerationChanged();
    Q_SIGNAL void pressDelayChanged();
    Q_SIGNAL void movementStarted();
    Q_SIGNAL void movementEnded();
    Q_SIGNAL void flickStarted();
    Q_SIGNAL void flickEnded();
    Q_SIGNAL void dragStarted();
    Q_SIGNAL void dragEnded();
    Q_SIGNAL void pixelAlignedChanged();
    Q_SIGNAL void synchronousDragChanged();
    Q_SIGNAL void atXEndChanged();
    Q_SIGNAL void atYEndChanged();
    Q_SIGNAL void atXBeginningChanged();
    Q_SIGNAL void atYBeginningChanged();
    Q_SIGNAL void acceptedButtonsChanged();
    Q_SIGNAL void inputMaskModeChanged();
    Q_SIGNAL void interactionItemChanged();

protected:
    auto childMouseEventFilter(QQuickItem* item, QEvent* event) -> bool override;
    auto mousePressEvent(QMouseEvent* event) -> void override;
    auto mouseMoveEvent(QMouseEvent* event) -> void override;
    auto mouseReleaseEvent(QMouseEvent* event) -> void override;
    auto touchEvent(QTouchEvent* event) -> void override;
    auto wheelEvent(QWheelEvent* event) -> void override;
    auto timerEvent(QTimerEvent* event) -> void override;
    auto geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) -> void override;
    auto componentComplete() -> void override;
    auto mouseUngrabEvent() -> void override;

protected:
    virtual auto minXExtent() const -> qreal;
    virtual auto minYExtent() const -> qreal;
    virtual auto maxXExtent() const -> qreal;
    virtual auto maxYExtent() const -> qreal;
    auto         vWidth() const -> qreal;
    auto         vHeight() const -> qreal;
    virtual auto viewportMoved(Qt::Orientations orientation) -> void;
    auto         xflick() const -> bool;
    auto         yflick() const -> bool;

private:
    friend class FlickableVisibleArea;

    enum Axis
    {
        HorizontalAxis,
        VerticalAxis,
    };

    enum MotionMode
    {
        NoMotion,
        FlickMotion,
    };

    struct AxisData {
        qreal                         viewSize { -1 };
        qreal                         startMargin { 0 };
        qreal                         endMargin { 0 };
        qreal                         pressPos { 0 };
        qreal                         pressContentPos { 0 };
        qreal                         lastPos { 0 };
        qreal                         dragStartOffset { 0 };
        qreal                         previousDragDelta { 0 };
        qreal                         velocity { 0 };
        qreal                         smoothVelocity { 0 };
        qreal                         motionStart { 0 };
        qreal                         motionTarget { 0 };
        qreal                         motionVelocity { 0 };
        qreal                         motionDuration { 0 };
        qreal                         motionElapsed { 0 };
        QVector<QPair<qint64, qreal>> velocityBuffer;
        MotionMode                    motionMode { NoMotion };
        bool                          atEnd { false };
        bool                          atBeginning { true };
        bool                          moving { false };
        bool                          flicking { false };
        bool                          dragging { false };

        auto resetDrag() -> void;
        auto addVelocitySample(qint64 timestamp, qreal position, qreal maxVelocity) -> void;
        auto updateVelocity() -> void;
    };

    static auto dataAppend(QQmlListProperty<QObject>* property, QObject* object) -> void;
    static auto dataCount(QQmlListProperty<QObject>* property) -> qsizetype;
    static auto dataAt(QQmlListProperty<QObject>* property, qsizetype index) -> QObject*;
    static auto dataClear(QQmlListProperty<QObject>* property) -> void;
    static auto childrenAppend(QQmlListProperty<QQuickItem>* property, QQuickItem* item) -> void;
    static auto childrenCount(QQmlListProperty<QQuickItem>* property) -> qsizetype;
    static auto childrenAt(QQmlListProperty<QQuickItem>* property, qsizetype index) -> QQuickItem*;
    static auto childrenClear(QQmlListProperty<QQuickItem>* property) -> void;

    auto axisData(Axis axis) -> AxisData&;
    auto axisData(Axis axis) const -> const AxisData&;
    auto axisPosition(Axis axis) const -> qreal;
    auto setAxisPosition(Axis axis, qreal position) -> void;
    auto minExtent(Axis axis) const -> qreal;
    auto maxExtent(Axis axis) const -> qreal;
    auto viewportSize(Axis axis) const -> qreal;
    auto axisCanFlick(Axis axis) const -> bool;
    auto aligned(qreal value) const -> qreal;
    auto boundedPosition(Axis axis, qreal position) const -> qreal;
    auto updateContentSize(Axis axis) -> void;
    auto updateBeginningEnd() -> void;
    auto updateVisibleArea() -> void;

    auto setAxisMoving(Axis axis, bool value) -> void;
    auto setAxisDragging(Axis axis, bool value) -> void;
    auto setAxisFlicking(Axis axis, bool value) -> void;
    auto setAxisVelocity(Axis axis, qreal value) -> void;
    auto movementStarting() -> void;
    auto movementEnding() -> void;
    auto flickingStarted(bool horizontal, bool vertical) -> void;
    auto draggingStarting(bool horizontal, bool vertical) -> void;
    auto draggingEnding() -> void;

    auto startAxisFlick(Axis axis, qreal velocity) -> void;
    auto stopAxisMotion(Axis axis) -> void;
    auto ensureMotionTimer() -> void;
    auto advanceAxis(Axis axis, qreal deltaSeconds) -> void;
    auto fixup(Axis axis) -> void;
    auto isAxisAnimating(Axis axis) const -> bool;

    auto handlePress(const QPointF& position, qint64 timestamp) -> void;
    auto handleMove(const QPointF& position, qint64 timestamp, Qt::MouseButtons buttons) -> void;
    auto handleRelease(const QPointF& position, qint64 timestamp) -> void;
    auto cancelInteraction() -> void;
    auto buttonsAccepted(const QSinglePointEvent* event) const -> bool;
    auto pointerAccepted(QQuickItem* receiver, QEvent* event) const -> bool;
    auto acceptsPoint(const QPointF& point) const -> bool;
    auto eventPosition(QQuickItem* receiver, QEvent* event) const -> QPointF;
    auto ignorePointerEvent(QEvent* event) const -> void;

private:
    QQuickItem*           m_contentItem { nullptr };
    QList<QObject*>       m_data;
    AxisData              m_hData;
    AxisData              m_vData;
    FlickableVisibleArea* m_visibleArea { nullptr };
    QPointer<QQuickItem>  m_interactionItem;
    QBasicTimer           m_motionTimer;
    QElapsedTimer         m_motionClock;
    QBasicTimer           m_pressDelayTimer;
    QPointF               m_pressPos;
    QPointF               m_lastPos;
    qint64                m_lastPosTime { -1 };
    qint64                m_lastPressTime { -1 };
    FlickableDirection    m_flickableDirection { AutoFlickDirection };
    InputMaskMode         m_inputMaskMode { Viewport };
    qreal                 m_deceleration { 5000 };
    qreal                 m_wheelDeceleration { 15000 };
    qreal                 m_maxVelocity { 2500 };
    int                   m_pressDelay { 0 };
    bool                  m_pressed { false };
    bool                  m_stealMouse { false };
    bool                  m_interactive { true };
    bool                  m_pixelAligned { false };
    bool                  m_syncDrag { false };
    Qt::MouseButtons      m_acceptedButtons { Qt::LeftButton };
};

} // namespace qml_material
