#pragma once

#include "qml_material/control/panel.hpp"

namespace qml_material
{
class QML_MATERIAL_API SideSheet : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(SideSheetBase)
    Q_CLASSINFO("DefaultProperty", "contentData")
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(Panel* sheetItem READ sheetItem CONSTANT FINAL)
    Q_PROPERTY(QQuickItem* mainContent READ contentItem WRITE setContentItem NOTIFY
                   contentItemChanged FINAL)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged FINAL)
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(State lastStableState READ lastStableState NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool dragging READ dragging NOTIFY stateChanged FINAL)
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(Edge edge READ edge WRITE setEdge NOTIFY configurationChanged FINAL)
    Q_PROPERTY(Qt::Edge effectiveEdge READ effectiveEdge NOTIFY configurationChanged FINAL)
    Q_PROPERTY(
        qreal sheetWidth READ sheetWidth WRITE setSheetWidth NOTIFY configurationChanged FINAL)
    Q_PROPERTY(qreal effectiveSheetWidth READ effectiveSheetWidth NOTIFY sheetGeometryChanged FINAL)
    Q_PROPERTY(qreal occupiedWidth READ occupiedWidth NOTIFY sheetGeometryChanged FINAL)
    Q_PROPERTY(bool detached READ detached WRITE setDetached NOTIFY configurationChanged FINAL)
    Q_PROPERTY(bool coplanar READ coplanar WRITE setCoplanar NOTIFY configurationChanged FINAL)
    Q_PROPERTY(bool draggable READ draggable WRITE setDraggable NOTIFY configurationChanged FINAL)
    Q_PROPERTY(qreal hideFriction READ hideFriction WRITE setHideFriction NOTIFY
                   configurationChanged FINAL)
public:
    enum Edge
    {
        Start,
        End,
        Left,
        Right
    };
    Q_ENUM(Edge)
    enum State
    {
        Hidden,
        Expanded,
        Dragging,
        Settling
    };
    Q_ENUM(State)
    explicit SideSheet(QQuickItem* parent = nullptr);
    QQmlListProperty<QObject> contentData() { return m_sheet->contentData(); }
    Panel*                    sheetItem() const { return m_sheet; }
    bool                      expanded() const { return m_expanded; }
    State                     state() const { return m_state; }
    State                     lastStableState() const { return m_lastStable; }
    bool                      dragging() const { return m_state == Dragging; }
    qreal                     position() const { return m_position; }
    Edge                      edge() const { return m_edge; }
    Qt::Edge                  effectiveEdge() const;
    qreal                     sheetWidth() const { return m_sheetWidth; }
    qreal                     effectiveSheetWidth() const;
    qreal                     occupiedWidth() const;
    bool                      detached() const { return m_detached; }
    bool                      coplanar() const { return m_coplanar; }
    bool                      draggable() const { return m_draggable; }
    qreal                     hideFriction() const { return m_hideFriction; }
    void                      setExpanded(bool);
    void                      setPosition(qreal);
    void                      setEdge(Edge);
    void                      setSheetWidth(qreal);
    void                      setDetached(bool);
    void                      setCoplanar(bool);
    void                      setDraggable(bool);
    void                      setHideFriction(qreal);
    Q_INVOKABLE void          open() { setExpanded(true); }
    Q_INVOKABLE void          close() { setExpanded(false); }
    Q_INVOKABLE bool          beginDrag();
    Q_INVOKABLE void          dragBy(QPointF delta);
    Q_INVOKABLE void          releaseDrag(QPointF velocity);
    Q_INVOKABLE void          cancelDrag();
    Q_INVOKABLE void          completeTransition(quint32 revision);
    Q_SIGNAL void             expandedChanged();
    Q_SIGNAL void             stateChanged();
    Q_SIGNAL void             positionChanged();
    Q_SIGNAL void             configurationChanged();
    Q_SIGNAL void             sheetGeometryChanged();
    Q_SIGNAL void             transitionRequested(quint32 revision, qreal position, bool animate);
    Q_SIGNAL void             opened();
    Q_SIGNAL void             closed();

protected:
    QRectF contentRect() const override;
    void   classBegin() override;
    void   componentComplete() override;
    void   geometryChange(const QRectF&, const QRectF&) override;

private:
    qreal   margin() const { return m_detached ? 16 : 0; }
    qreal   hiddenX() const;
    qreal   expandedX() const;
    qreal   sheetX() const;
    bool    releaseOpens(QPointF velocity) const;
    void    request(bool expanded, bool animate);
    void    updateLayout();
    void    configurationUpdated();
    Panel*  m_sheet;
    Edge    m_edge         = End;
    State   m_state        = Hidden;
    State   m_lastStable   = Hidden;
    qreal   m_position     = 0;
    qreal   m_sheetWidth   = 256;
    qreal   m_hideFriction = 0.1;
    bool    m_expanded     = false;
    bool    m_detached     = false;
    bool    m_coplanar     = false;
    bool    m_draggable    = true;
    bool    m_initializing = false;
    bool    m_layoutBusy   = false;
    bool    m_layoutDirty  = false;
    quint32 m_revision     = 0;
};
} // namespace qml_material
