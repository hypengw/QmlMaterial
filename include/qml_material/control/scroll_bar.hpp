#pragma once

#include "qml_material/control/scroll_indicator.hpp"
#include "qml_material/input/input_host.hpp"

namespace qml_material
{
class ScrollBarAttached;

/** @ingroup control */
class QML_MATERIAL_API ScrollBar : public ScrollIndicator {
    Q_OBJECT
    QML_NAMED_ELEMENT(ScrollBarBase)
    QML_ATTACHED(ScrollBarAttached)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY interactionSourceChanged FINAL)
    Q_PROPERTY(qml_material::InteractionValues interactionValues READ interactionValues NOTIFY
                   interactionValuesChanged FINAL)
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive RESET resetInteractive NOTIFY
                   interactiveChanged FINAL)
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged FINAL)
    Q_PROPERTY(Policy policy READ policy WRITE setPolicy NOTIFY policyChanged FINAL)
public:
    enum SnapMode
    {
        NoSnap,
        SnapAlways,
        SnapOnRelease
    };
    Q_ENUM(SnapMode)
    enum Policy
    {
        AsNeeded  = Qt::ScrollBarAsNeeded,
        AlwaysOff = Qt::ScrollBarAlwaysOff,
        AlwaysOn  = Qt::ScrollBarAlwaysOn
    };
    Q_ENUM(Policy)
    explicit ScrollBar(QQuickItem* parent = nullptr);
    ~ScrollBar() override;
    static ScrollBarAttached* qmlAttachedProperties(QObject*);
    qreal                     stepSize() const { return m_step; }
    InteractionSource*        interactionSource() { return m_input.source(); }
    void              setInteractionSource(InteractionSource* source) { m_input.setSource(source); }
    InteractionValues interactionValues() const { return m_feedback.values(); }
    bool              interactive() const { return m_interactive; }
    SnapMode          snapMode() const { return m_snap; }
    Policy            policy() const { return m_policy; }
    void              setStepSize(qreal);
    void              setInteractive(bool);
    void              resetInteractive();
    void              setSnapMode(SnapMode);
    void              setPolicy(Policy);
    Q_INVOKABLE void  increase();
    Q_INVOKABLE void  decrease();
    Q_INVOKABLE void  requestPosition(qreal);
    Q_SIGNAL void     stepSizeChanged();
    Q_SIGNAL void     interactionSourceChanged();
    Q_SIGNAL void     interactionValuesChanged();
    Q_SIGNAL void     positionChangeRequested(qreal position);
    Q_SIGNAL void     interactiveChanged();
    Q_SIGNAL void     snapModeChanged();
    Q_SIGNAL void     policyChanged();

protected:
    void setMoving(bool) override;
    void disconnectFlickable() override;
    bool event(QEvent*) override;
    void mouseUngrabEvent() override;
    void touchUngrabEvent() override;

private:
    void                   updateActive();
    void                   cancel();
    void                   begin(const QPointF&);
    void                   move(const QPointF&, bool release);
    void                   step(qreal);
    qreal                  logicalPosition(qreal) const;
    qreal                  positionAt(const QPointF&) const;
    InputHost              m_input;
    InteractionAccumulator m_feedback;
    qreal                  m_step   = 0;
    qreal                  m_offset = 0;
    QPointF                m_dragPoint;
    bool                   m_interactive = true;
    bool                   m_moving      = false;
    SnapMode               m_snap        = NoSnap;
    Policy                 m_policy      = AsNeeded;
};

class QML_MATERIAL_API ScrollBarAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(
        ScrollBar* horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged FINAL)
    Q_PROPERTY(ScrollBar* vertical READ vertical WRITE setVertical NOTIFY verticalChanged FINAL)
public:
    explicit ScrollBarAttached(QObject* parent);
    ScrollBar*    horizontal() const;
    ScrollBar*    vertical() const;
    void          setHorizontal(ScrollBar*);
    void          setVertical(ScrollBar*);
    Q_SIGNAL void horizontalChanged();
    Q_SIGNAL void verticalChanged();

private:
    ScrollIndicatorAttached* m_attachment;
};
} // namespace qml_material

QML_DECLARE_TYPEINFO(qml_material::ScrollBar, QML_HAS_ATTACHED_PROPERTIES)
