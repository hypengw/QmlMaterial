#pragma once

#include "qml_material/control/popup.hpp"
#include "qml_material/item/item_proxy.hpp"

namespace qml_material
{
class QML_MATERIAL_API Drawer : public Popup {
    Q_OBJECT
    QML_NAMED_ELEMENT(DrawerBase)
    Q_PROPERTY(Qt::Edge edge READ edge WRITE setEdge NOTIFY edgeChanged FINAL)
    Q_PROPERTY(
        RevealMode revealMode READ revealMode WRITE setRevealMode NOTIFY revealModeChanged FINAL)
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(
        bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged FINAL)
    Q_PROPERTY(qreal dragMargin READ dragMargin WRITE setDragMargin RESET resetDragMargin NOTIFY
                   dragMarginChanged FINAL)
public:
    enum RevealMode
    {
        Slide,
        Expand
    };
    Q_ENUM(RevealMode)
    RevealMode    revealMode() const { return m_revealMode; }
    void          setRevealMode(RevealMode);
    bool          prepareExpansion(QObject* owner, ItemProxy* origin);
    void          clearExpansion(QObject* owner);
    Q_SIGNAL void revealModeChanged();
    explicit Drawer(QObject* parent = nullptr);
    ~Drawer() override;
    bool          interactive() const { return m_interactive; }
    void          setInteractive(bool);
    qreal         dragMargin() const { return m_dragMargin; }
    void          setDragMargin(qreal);
    void          resetDragMargin();
    bool          acceptsDrag(const QPointF&) const;
    bool          wantsDrag(const QPointF&) const;
    void          pressDrag(const QPointF&, ulong timestamp);
    void          startDrag(const QPointF&);
    void          moveDrag(const QPointF&);
    void          releaseDrag(const QPointF&, ulong timestamp);
    void          cancelDrag();
    bool          dragging() const { return m_dragging; }
    void          open() override;
    void          close() override;
    void          dismissImmediately() override;
    Qt::Edge      edge() const { return m_edge; }
    void          setEdge(Qt::Edge);
    qreal         position() const { return m_position; }
    void          setPosition(qreal);
    void          updateDimmer(QQuickItem*, qreal opacity) const override;
    bool          blocksScenePoint(const QPointF&) const override;
    bool          overlayContainsScenePoint(const QPointF&) const override;
    Q_SIGNAL void edgeChanged();
    Q_SIGNAL void positionChanged();
    Q_SIGNAL void interactiveChanged();
    Q_SIGNAL void dragMarginChanged();

protected:
    QPointF                 surfacePosition() const override;
    QSizeF                  surfaceSize() const override;
    qreal                   presentationScale() const override;
    QList<TransitionTarget> transitionTargets(bool opening) const override;
    void                    finalizeTransition(bool opening) override;

private:
    void                           registerOverlay();
    void                           observeOrigin();
    QRectF                         expansionRect() const;
    ItemProxy::Geometry            expansionGeometry() const;
    ItemProxy::Geometry            originGeometry() const;
    QPointer<ItemProxy>            m_origin;
    QList<QMetaObject::Connection> m_originConnections;
    ItemProxy::Geometry            m_originGeometry;
    mutable ItemProxy::Geometry    m_startGeometry, m_endGeometry, m_anchorGeometry;
    mutable qreal                  m_anchorProgress = 0;
    mutable bool                   m_geometryReady  = false;
    RevealMode                     m_revealMode     = Slide;
    qreal                          axis(const QPointF&) const;
    qreal                          extent() const;
    qreal                          distanceFromEdge(const QPointF&) const;
    QPointer<OverlayManager>       m_inputOverlay;
    QPointF                        m_pressPoint;
    ulong                          m_pressTimestamp = 0;
    qreal                          m_dragOrigin = 0, m_dragPosition = 0, m_dragMargin = 0;
    bool                           m_interactive = true, m_dragging = false, m_wasOpen = false;
    Qt::Edge                       m_edge     = Qt::LeftEdge;
    qreal                          m_position = 0;
};
} // namespace qml_material
