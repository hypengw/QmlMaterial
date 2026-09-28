#pragma once

#include "qml_material/control/control.hpp"

namespace qml_material
{
class RangeSlider;

class QML_MATERIAL_API RangeSliderNode : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool focused READ focused NOTIFY focusedChanged FINAL)
    Q_PROPERTY(QQuickItem* handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    Q_PROPERTY(
        qreal implicitHandleWidth READ implicitHandleWidth NOTIFY implicitHandleSizeChanged FINAL)
    Q_PROPERTY(
        qreal implicitHandleHeight READ implicitHandleHeight NOTIFY implicitHandleSizeChanged FINAL)
public:
    ~RangeSliderNode() override;
    qreal            visualPosition() const;
    bool             pressed() const;
    bool             focused() const;
    QQuickItem*      handle() const { return m_handle; }
    void             setHandle(QQuickItem*);
    qreal            implicitHandleWidth() const;
    qreal            implicitHandleHeight() const;
    qreal            value() const;
    qreal            position() const;
    void             setValue(qreal);
    Q_INVOKABLE void increase();
    Q_INVOKABLE void decrease();
    Q_SIGNAL void    valueChanged();
    Q_SIGNAL void    positionChanged();
    Q_SIGNAL void    visualPositionChanged();
    Q_SIGNAL void    pressedChanged();
    Q_SIGNAL void    focusedChanged();
    Q_SIGNAL void    handleChanged();
    Q_SIGNAL void    implicitHandleSizeChanged();
    Q_SIGNAL void    moved();

private:
    friend class RangeSlider;
    RangeSliderNode(RangeSlider*, int);
    RangeSlider*                   m_owner;
    int                            m_index;
    QPointer<QQuickItem>           m_handle;
    QList<QMetaObject::Connection> m_handleConnections;
    quint64                        m_handleRevision = 0;
};

/** @ingroup control */
class QML_MATERIAL_API RangeSlider : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(RangeSliderBase)
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(
        qreal minimumRange READ minimumRange WRITE setMinimumRange NOTIFY minimumRangeChanged FINAL)
    Q_PROPERTY(qreal effectiveMinimumRange READ effectiveMinimumRange NOTIFY
                   effectiveMinimumRangeChanged FINAL)
    Q_PROPERTY(qml_material::RangeSliderNode* first READ first CONSTANT FINAL)
    Q_PROPERTY(qml_material::RangeSliderNode* second READ second CONSTANT FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged FINAL)
    Q_PROPERTY(bool horizontal READ horizontal NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool vertical READ vertical NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged FINAL)
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged FINAL)
    Q_PROPERTY(
        bool wheelEnabled READ wheelEnabled WRITE setWheelEnabled NOTIFY wheelEnabledChanged FINAL)
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(int activeHandle READ activeHandle NOTIFY activeHandleChanged FINAL)
    Q_PROPERTY(int focusedHandle READ focusedHandle WRITE setFocusedHandle NOTIFY
                   focusedHandleChanged FINAL)
    Q_PROPERTY(qreal trackStart READ trackStart NOTIFY trackGeometryChanged FINAL)
    Q_PROPERTY(qreal trackLength READ trackLength NOTIFY trackGeometryChanged FINAL)
public:
    enum SnapMode
    {
        NoSnap,
        SnapAlways,
        SnapOnRelease
    };
    Q_ENUM(SnapMode)
    explicit RangeSlider(QQuickItem* parent = nullptr);
    ~RangeSlider() override;
    Qt::Orientation   orientation() const;
    bool              horizontal() const { return orientation() == Qt::Horizontal; }
    bool              vertical() const { return ! horizontal(); }
    bool              live() const;
    SnapMode          snapMode() const;
    bool              wheelEnabled() const;
    bool              pressed() const { return activeHandle() >= 0; }
    int               activeHandle() const;
    int               focusedHandle() const;
    qreal             trackStart() const;
    qreal             trackLength() const;
    void              setOrientation(Qt::Orientation);
    void              setLive(bool);
    void              setSnapMode(SnapMode);
    void              setWheelEnabled(bool);
    void              setFocusedHandle(int);
    Q_INVOKABLE qreal positionAt(const QPointF&) const;
    Q_SIGNAL void     orientationChanged();
    Q_SIGNAL void     liveChanged();
    Q_SIGNAL void     snapModeChanged();
    Q_SIGNAL void     wheelEnabledChanged();
    Q_SIGNAL void     pressedChanged();
    Q_SIGNAL void     activeHandleChanged();
    Q_SIGNAL void     focusedHandleChanged();
    Q_SIGNAL void     trackGeometryChanged();
    Q_SIGNAL void     interactionStarted(int handle);
    Q_SIGNAL void     interactionFinished(int handle, bool cancelled);
    qreal             from() const;
    qreal             to() const;
    qreal             stepSize() const;
    qreal             minimumRange() const;
    qreal             effectiveMinimumRange() const;
    RangeSliderNode*  first() { return &m_first; }
    RangeSliderNode*  second() { return &m_second; }
    void              setFrom(qreal);
    void              setTo(qreal);
    void              setStepSize(qreal);
    void              setMinimumRange(qreal);
    Q_INVOKABLE void  setValues(qreal first, qreal second);
    Q_INVOKABLE qreal valueAt(qreal position) const;
    Q_INVOKABLE QList<qreal> tickPositions(int maximumCount) const;
    Q_SIGNAL void            fromChanged();
    Q_SIGNAL void            toChanged();
    Q_SIGNAL void            stepSizeChanged();
    Q_SIGNAL void            minimumRangeChanged();
    Q_SIGNAL void            effectiveMinimumRangeChanged();

protected:
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
    void focusInEvent(QFocusEvent*) override;
    void itemChange(ItemChange, const ItemChangeData&) override;
    void classBegin() override;
    void componentComplete() override;

private:
    friend class RangeSliderNode;
    struct State {
        qreal           from = 0, to = 1, step = 0, minimum = 0, effective = 0;
        qreal           values[2] { 0, 1 };
        qreal           positions[2] { 0, 1 };
        Qt::Orientation orientation = Qt::Horizontal;
        SnapMode        snap        = NoSnap;
        bool            live = true, wheel = false;
        int             active = -1, focused = 0;
        bool            operator==(const State&) const = default;
    };
    static qreal span(const State&);
    static qreal distance(const State&, qreal value);
    static qreal value(const State&, qreal distance);
    static qreal nearest(const State&, qreal requested, qreal lower, qreal upper);
    static void  normalize(State&);
    void         publish(State);
    void         setNodeValue(int, qreal);
    qreal        nodeValue(int) const;
    qreal        nodePosition(int) const;
    enum class Input
    {
        None,
        Mouse,
        Touch,
        Key
    };
    bool                    prepareChange();
    bool                    finish(bool cancelled);
    bool                    activate(int index);
    void                    moveTo(qreal position, bool release);
    int                     pickHandle(qreal position, bool release) const;
    void                    observeWindow();
    bool                    inverted() const { return vertical() || mirrored(); }
    RangeSliderNode*        node(int index) { return index == 0 ? &m_first : &m_second; }
    qreal                   handleExtent(int index) const;
    Input                   m_input   = Input::None;
    int                     m_touchId = -1, m_key = 0;
    QPointF                 m_pressPoint;
    qreal                   m_pressPosition = 0;
    quint64                 m_sequence      = 0;
    bool                    m_started       = false;
    QMetaObject::Connection m_windowConnection;
    QProperty<State>        m_state { State {} };
    RangeSliderNode         m_first, m_second;
    bool                    m_initializing = false;
    unsigned                m_pending      = 0;
    bool                    m_notifying    = false;
};
} // namespace qml_material
