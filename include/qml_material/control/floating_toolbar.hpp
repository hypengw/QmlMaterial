#pragma once

#include "qml_material/control/control.hpp"
#include "qml_material/input/input_host.hpp"
#include "qml_material/input/floating_toolbar_exit.hpp"
#include <QVariantAnimation>

namespace qml_material
{
/** @ingroup control */
class QML_MATERIAL_API FloatingToolbar : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(FloatingToolbarBase)
    Q_PROPERTY(FloatingToolbarExit* exitBehavior READ exitBehavior WRITE setExitBehavior NOTIFY
                   exitBehaviorChanged FINAL)
    Q_PROPERTY(
        Qt::Edge exitEdge READ exitEdge WRITE setExitEdge NOTIFY exitConfigurationChanged FINAL)
    Q_PROPERTY(
        bool dragToHide READ dragToHide WRITE setDragToHide NOTIFY exitConfigurationChanged FINAL)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY
                   animationsEnabledChanged FINAL)
    Q_PROPERTY(qreal exitFlickDeceleration READ exitFlickDeceleration WRITE setExitFlickDeceleration
                   NOTIFY exitConfigurationChanged FINAL)
    Q_PROPERTY(qreal exitMaximumFlickVelocity READ exitMaximumFlickVelocity WRITE
                   setExitMaximumFlickVelocity NOTIFY exitConfigurationChanged FINAL)
    Q_PROPERTY(bool exitFlingRunning READ exitFlingRunning NOTIFY exitFlingChanged FINAL)
    Q_PROPERTY(qreal presentedExitOffset READ presentedExitOffset WRITE setPresentedExitOffset
                   NOTIFY presentedExitOffsetChanged FINAL)
public:
    explicit FloatingToolbar(QQuickItem* parent = nullptr);
    ~FloatingToolbar() override;
    FloatingToolbarExit* exitBehavior() const { return m_exit; }
    Qt::Edge             exitEdge() const { return m_edge; }
    bool                 dragToHide() const { return m_dragToHide; }
    bool                 animationsEnabled() const { return m_animationsEnabled; }
    qreal                exitFlickDeceleration() const { return m_deceleration; }
    qreal                exitMaximumFlickVelocity() const { return m_maximumVelocity; }
    bool  exitFlingRunning() const { return m_fling.state() == QAbstractAnimation::Running; }
    qreal presentedExitOffset() const { return m_presented; }
    void  setExitBehavior(FloatingToolbarExit*);
    void  setExitEdge(Qt::Edge);
    void  setDragToHide(bool);
    void  setAnimationsEnabled(bool);
    void  setExitFlickDeceleration(qreal);
    void  setExitMaximumFlickVelocity(qreal);
    void  setPresentedExitOffset(qreal);
    void  releaseExit(qreal velocity);
    Q_SIGNAL void exitBehaviorChanged();
    Q_SIGNAL void exitConfigurationChanged();
    Q_SIGNAL void animationsEnabledChanged();
    Q_SIGNAL void exitFlingChanged();
    Q_SIGNAL void presentedExitOffsetChanged();
    Q_SIGNAL void exitInputStarted();

private:
    qreal                          axis(QPointF) const;
    void                           movePointer(QPointF);
    void                           cancelExit();
    QPointer<FloatingToolbarExit>  m_exit;
    QList<QMetaObject::Connection> m_connections;
    Qt::Edge                       m_edge = Qt::BottomEdge;
    bool              m_dragToHide = true, m_animationsEnabled = true, m_dragging = false;
    qreal             m_deceleration = 1500, m_maximumVelocity = 2500, m_presented = 0;
    quint64           m_revision = 0;
    QPointF           m_lastScene;
    QVariantAnimation m_fling;
    InputHost         m_input;
};
} // namespace qml_material
