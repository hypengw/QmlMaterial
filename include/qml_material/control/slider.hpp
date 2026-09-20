#pragma once

#include "qml_material/control/control.hpp"

namespace qml_material
{

/** @ingroup control */
class QML_MATERIAL_API Slider : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(SliderBase)
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged FINAL)
    Q_PROPERTY(
        bool wheelEnabled READ wheelEnabled WRITE setWheelEnabled NOTIFY wheelEnabledChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(bool horizontal READ horizontal NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool vertical READ vertical NOTIFY orientationChanged FINAL)
    Q_PROPERTY(QQuickItem* handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    Q_PROPERTY(
        qreal implicitHandleWidth READ implicitHandleWidth NOTIFY implicitHandleWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHandleHeight READ implicitHandleHeight NOTIFY
                   implicitHandleHeightChanged FINAL)
    Q_PROPERTY(qreal touchDragThreshold READ touchDragThreshold WRITE setTouchDragThreshold RESET
                   resetTouchDragThreshold NOTIFY touchDragThresholdChanged FINAL)
public:
    enum SnapMode
    {
        NoSnap,
        SnapAlways,
        SnapOnRelease
    };
    Q_ENUM(SnapMode)
    explicit Slider(QQuickItem* parent = nullptr);
    ~Slider() override;
    qreal from() const { return m_from; }
    qreal to() const { return m_to; }
    qreal value() const { return m_value; }
    qreal position() const { return m_position; }
    qreal visualPosition() const { return vertical() || mirrored() ? 1 - m_position : m_position; }
    qreal stepSize() const { return m_step; }
    SnapMode          snapMode() const { return m_snap; }
    bool              pressed() const { return m_pressed; }
    bool              live() const { return m_live; }
    bool              wheelEnabled() const { return m_wheel_enabled; }
    Qt::Orientation   orientation() const { return m_orientation; }
    bool              horizontal() const { return m_orientation == Qt::Horizontal; }
    bool              vertical() const { return m_orientation == Qt::Vertical; }
    QQuickItem*       handle() const { return m_handle; }
    qreal             implicitHandleWidth() const;
    qreal             implicitHandleHeight() const;
    qreal             touchDragThreshold() const { return m_touch_threshold; }
    void              setFrom(qreal);
    void              setTo(qreal);
    void              setValue(qreal);
    void              setStepSize(qreal);
    void              setSnapMode(SnapMode);
    void              setPressed(bool);
    void              setLive(bool);
    void              setWheelEnabled(bool);
    void              setOrientation(Qt::Orientation);
    void              setHandle(QQuickItem*);
    void              setTouchDragThreshold(qreal);
    void              resetTouchDragThreshold();
    Q_INVOKABLE qreal valueAt(qreal position) const;
    Q_INVOKABLE void  increase();
    Q_INVOKABLE void  decrease();
    Q_SIGNAL void     fromChanged();
    Q_SIGNAL void     toChanged();
    Q_SIGNAL void     valueChanged();
    Q_SIGNAL void     positionChanged();
    Q_SIGNAL void     visualPositionChanged();
    Q_SIGNAL void     stepSizeChanged();
    Q_SIGNAL void     snapModeChanged();
    Q_SIGNAL void     pressedChanged();
    Q_SIGNAL void     liveChanged();
    Q_SIGNAL void     wheelEnabledChanged();
    Q_SIGNAL void     orientationChanged();
    Q_SIGNAL void     handleChanged();
    Q_SIGNAL void     implicitHandleWidthChanged();
    Q_SIGNAL void     implicitHandleHeightChanged();
    Q_SIGNAL void     touchDragThresholdChanged();
    Q_SIGNAL void     moved();

protected:
    void componentComplete() override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void touchEvent(QTouchEvent*) override;
    void touchUngrabEvent() override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void itemChange(ItemChange, const ItemChangeData&) override;

private:
    enum class Input
    {
        None,
        Mouse,
        Touch,
        Key
    };
    void                           setPosition(qreal);
    void                           updatePosition();
    qreal                          positionAt(const QPointF&) const;
    qreal                          snapPosition(qreal) const;
    void                           moveTo(const QPointF&, bool release);
    void                           cancel();
    void                           observeWindow();
    qreal                          m_from = 0, m_to = 1, m_value = 0, m_position = 0, m_step = 0;
    qreal                          m_touch_threshold = -1;
    Qt::Orientation                m_orientation     = Qt::Horizontal;
    SnapMode                       m_snap            = NoSnap;
    bool                           m_pressed = false, m_live = true, m_wheel_enabled = false;
    Input                          m_input    = Input::None;
    int                            m_touch_id = -1, m_key = 0;
    QPointF                        m_press_point;
    QPointer<QQuickItem>           m_handle;
    QList<QMetaObject::Connection> m_handle_connections;
    QMetaObject::Connection        m_window_connection;
    quint64                        m_sequence = 0;
};

} // namespace qml_material
