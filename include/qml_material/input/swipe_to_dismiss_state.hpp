#pragma once

#include <QObject>
#include <qqmlregistration.h>
#include "qml_material/export.hpp"
#include "qml_material/input/input_host.hpp"
Q_MOC_INCLUDE("qml_material/control/control.hpp")

namespace qml_material
{
class Control;
// Logical anchors and targets; the presenter owns animation and acknowledges its revision.
class QML_MATERIAL_API SwipeToDismissState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal distance READ distance WRITE setDistance NOTIFY configurationChanged FINAL)
    Q_PROPERTY(bool startToEndEnabled READ startToEndEnabled WRITE setStartToEndEnabled NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(bool endToStartEnabled READ endToStartEnabled WRITE setEndToStartEnabled NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(qreal positionalThreshold READ positionalThreshold WRITE setPositionalThreshold
                   NOTIFY configurationChanged FINAL)
    Q_PROPERTY(qreal velocityThreshold READ velocityThreshold WRITE setVelocityThreshold NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(qreal offset READ offset NOTIFY motionChanged FINAL)
    Q_PROPERTY(qreal progress READ progress NOTIFY motionChanged FINAL)
    Q_PROPERTY(Value targetValue READ targetValue NOTIFY motionChanged FINAL)
    Q_PROPERTY(Value settledValue READ settledValue NOTIFY motionChanged FINAL)
    Q_PROPERTY(Value dismissDirection READ dismissDirection NOTIFY motionChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY motionChanged FINAL)
    Q_PROPERTY(bool settling READ settling NOTIFY motionChanged FINAL)
    Q_PROPERTY(Control* inputItem READ inputItem WRITE setInputItem NOTIFY inputItemChanged FINAL)
    Q_PROPERTY(bool gesturesEnabled READ gesturesEnabled WRITE setGesturesEnabled NOTIFY
                   inputItemChanged FINAL)
    Q_PROPERTY(qreal presentedOffset READ presentedOffset WRITE setPresentedOffset NOTIFY
                   presentedOffsetChanged FINAL)
public:
    enum Value
    {
        Settled,
        StartToEnd,
        EndToStart
    };
    Q_ENUM(Value)
    explicit SwipeToDismissState(QObject* parent = nullptr);
    ~SwipeToDismissState() override;
    Control*         inputItem() const;
    void             setInputItem(Control*);
    bool             gesturesEnabled() const { return m_gesturesEnabled; }
    void             setGesturesEnabled(bool);
    qreal            presentedOffset() const { return m_presentedOffset; }
    void             setPresentedOffset(qreal);
    Q_SIGNAL void    inputItemChanged();
    Q_SIGNAL void    presentedOffsetChanged();
    qreal            distance() const { return m_distance; }
    bool             startToEndEnabled() const { return m_startToEnd; }
    bool             endToStartEnabled() const { return m_endToStart; }
    qreal            positionalThreshold() const { return m_positionalThreshold; }
    qreal            velocityThreshold() const { return m_velocityThreshold; }
    qreal            offset() const { return m_offset; }
    qreal            progress() const;
    Value            targetValue() const { return m_target; }
    Value            settledValue() const { return m_settled; }
    Value            dismissDirection() const;
    bool             dragging() const { return m_dragging; }
    bool             settling() const { return m_settling; }
    void             setDistance(qreal);
    void             setStartToEndEnabled(bool);
    void             setEndToStartEnabled(bool);
    void             setPositionalThreshold(qreal);
    void             setVelocityThreshold(qreal);
    Q_INVOKABLE void dismiss(Value);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void snapTo(Value);
    Q_INVOKABLE bool begin(qreal presentedOffset);
    Q_INVOKABLE void dragBy(qreal delta);
    Q_INVOKABLE void release(qreal velocity);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void complete(quint32 revision);
    Q_SIGNAL void    configurationChanged();
    Q_SIGNAL void    motionChanged();
    Q_SIGNAL void    transitionRequested(quint32 revision, qreal offset, bool animate);
    Q_SIGNAL void    dismissed(Value direction);

private:
    bool                           allowed(Value) const;
    qreal                          position(Value) const;
    qreal                          bounded(qreal) const;
    Value                          releaseTarget(qreal velocity) const;
    void                           request(Value, bool animate);
    void                           anchorsChanged();
    qreal                          m_distance            = 0;
    qreal                          m_positionalThreshold = 56;
    qreal                          m_velocityThreshold   = 125;
    qreal                          m_offset              = 0;
    Value                          m_target              = Settled;
    Value                          m_settled             = Settled;
    bool                           m_startToEnd          = true;
    bool                           m_endToStart          = true;
    bool                           m_dragging            = false;
    bool                           m_settling            = false;
    quint32                        m_revision            = 0;
    QPointer<Control>              m_inputItem;
    std::unique_ptr<InputHost>     m_input;
    QList<QMetaObject::Connection> m_inputConnections;
    QPointF                        m_lastPoint;
    bool                           m_gesturesEnabled = true, m_dragAccepted = false;
    qreal                          m_presentedOffset = 0;
    quint64                        m_inputGeneration = 0;
};
} // namespace qml_material
