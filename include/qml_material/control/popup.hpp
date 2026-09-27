#pragma once

#include "qml_material/export.hpp"
#include <QFont>
#include <QLocale>
#include <QPointer>
#include <QQuickItem>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include <QtQml/qqmlregistration.h>
#include <QTimer>
#include <optional>
#include <memory>
#include <functional>

QT_BEGIN_NAMESPACE
class QQuickTransition;
QT_END_NAMESPACE

Q_MOC_INCLUDE("qml_material/control/panel.hpp")
Q_MOC_INCLUDE("QtQuick/private/qquicktransition_p.h")

namespace qml_material
{
class Control;
class Panel;
class OverlayManager;
class PopupMotion;

class QML_MATERIAL_API Popup : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_NAMED_ELEMENT(PopupBase)
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "contentData")
    Q_PROPERTY(QQuickItem* parent READ parentItem WRITE setParentItem NOTIFY parentChanged FINAL)
    Q_PROPERTY(QQuickItem* positioningItem READ positioningItem WRITE setPositioningItem NOTIFY
                   positioningItemChanged FINAL)
    Q_PROPERTY(QQuickItem* surfaceItem READ surfaceItem CONSTANT FINAL)
    Q_PROPERTY(
        QQuickItem* popupItem READ popupItem WRITE setPopupItem NOTIFY popupItemChanged FINAL)
    Q_PROPERTY(QQuickItem* overlayItem READ overlayItem NOTIFY overlayItemChanged FINAL)
    Q_PROPERTY(qreal overlayWidth READ overlayWidth NOTIFY overlayGeometryChanged FINAL)
    Q_PROPERTY(qreal overlayHeight READ overlayHeight NOTIFY overlayGeometryChanged FINAL)
    Q_PROPERTY(QRectF presentationRect READ presentationRect NOTIFY presentationRectChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickItem> contentChildren READ contentChildren NOTIFY
                   contentChildrenChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool opened READ isOpened NOTIFY openedChanged FINAL)
    Q_PROPERTY(bool entering READ entering NOTIFY enteringChanged FINAL)
    Q_PROPERTY(bool closing READ closing NOTIFY closingChanged FINAL)
    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged FINAL)
    Q_PROPERTY(bool dim READ dim WRITE setDim RESET resetDim NOTIFY dimChanged FINAL)
    Q_PROPERTY(bool focus READ focus WRITE setFocus NOTIFY focusChanged FINAL)
    Q_PROPERTY(bool deferredCompletion READ deferredCompletion WRITE setDeferredCompletion NOTIFY
                   deferredCompletionChanged FINAL)
    Q_PROPERTY(int hideDelay READ hideDelay WRITE setHideDelay NOTIFY hideDelayChanged FINAL)
    Q_PROPERTY(ClosePolicy closePolicy READ closePolicy WRITE setClosePolicy NOTIFY
                   closePolicyChanged FINAL)
    Q_PROPERTY(CollisionPolicy collisionPolicy READ collisionPolicy WRITE setCollisionPolicy NOTIFY
                   collisionPolicyChanged FINAL)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal z READ z WRITE setZ RESET resetZ NOTIFY zChanged FINAL)
    Q_PROPERTY(qreal margins READ margins WRITE setMargins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin RESET resetLeftMargin NOTIFY
                   marginsChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin RESET resetRightMargin NOTIFY
                   marginsChanged FINAL)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin RESET resetTopMargin NOTIFY
                   marginsChanged FINAL)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin RESET resetBottomMargin
                   NOTIFY marginsChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(
        QLocale locale READ locale WRITE setLocale RESET resetLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(QQuickTransition* enter READ enter WRITE setEnter NOTIFY enterChanged FINAL)
    Q_PROPERTY(QQuickTransition* exit READ exit WRITE setExit NOTIFY exitChanged FINAL)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem WRITE setContentItem NOTIFY
                   contentItemChanged FINAL)
    Q_PROPERTY(
        QQuickItem* background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth RESET resetWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(
        qreal height READ height WRITE setHeight RESET resetHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY
                   implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY
                   implicitHeightChanged FINAL)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth RESET resetContentWidth
                   NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight RESET
                   resetContentHeight NOTIFY contentHeightChanged FINAL)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal horizontalPadding READ horizontalPadding WRITE setHorizontalPadding NOTIFY
                   horizontalPaddingChanged FINAL)
    Q_PROPERTY(qreal verticalPadding READ verticalPadding WRITE setVerticalPadding NOTIFY
                   verticalPaddingChanged FINAL)
    Q_PROPERTY(
        qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(
        qreal rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY
                   bottomPaddingChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset NOTIFY leftInsetChanged FINAL)
    Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset NOTIFY rightInsetChanged FINAL)
    Q_PROPERTY(qreal topInset READ topInset WRITE setTopInset NOTIFY topInsetChanged FINAL)
    Q_PROPERTY(
        qreal bottomInset READ bottomInset WRITE setBottomInset NOTIFY bottomInsetChanged FINAL)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged FINAL)
    Q_PROPERTY(QQuickItem::TransformOrigin transformOrigin READ transformOrigin WRITE
                   setTransformOrigin NOTIFY transformOriginChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(qreal availableWidth READ availableWidth NOTIFY availableWidthChanged FINAL)
    Q_PROPERTY(qreal availableHeight READ availableHeight NOTIFY availableHeightChanged FINAL)
    Q_PROPERTY(qreal implicitContentWidth READ implicitContentWidth NOTIFY
                   implicitContentWidthChanged FINAL)
    Q_PROPERTY(qreal implicitContentHeight READ implicitContentHeight NOTIFY
                   implicitContentHeightChanged FINAL)
    Q_PROPERTY(qreal implicitBackgroundWidth READ implicitBackgroundWidth NOTIFY
                   implicitBackgroundWidthChanged FINAL)
    Q_PROPERTY(qreal implicitBackgroundHeight READ implicitBackgroundHeight NOTIFY
                   implicitBackgroundHeightChanged FINAL)
    Q_PROPERTY(bool activeFocus READ activeFocus NOTIFY activeFocusChanged FINAL)
    Q_PROPERTY(bool visualFocus READ visualFocus NOTIFY visualFocusChanged FINAL)
    Q_PROPERTY(bool mirrored READ mirrored NOTIFY mirroredChanged FINAL)
public:
    explicit Popup(QObject* parent = nullptr);
    ~Popup() override;
    enum ClosePolicyFlag
    {
        NoAutoClose                 = 0,
        CloseOnPressOutside         = 1,
        CloseOnPressOutsideParent   = 2,
        CloseOnReleaseOutside       = 4,
        CloseOnReleaseOutsideParent = 8,
        CloseOnEscape               = 16
    };
    Q_DECLARE_FLAGS(ClosePolicy, ClosePolicyFlag)
    Q_FLAG(ClosePolicy)
    enum CollisionPolicy
    {
        Unrestricted,
        Clamp
    };
    Q_ENUM(CollisionPolicy)
    QQuickItem* parentItem() const { return m_parent; }
    void        setParentItem(QQuickItem*);
    // Null uses the logical parent; changing the coordinate reference does not reparent the popup.
    QQuickItem*                  positioningItem() const { return m_positioningItem; }
    void                         setPositioningItem(QQuickItem*);
    Panel*                       surfaceItem() const { return m_surface; }
    QQuickItem*                  popupItem() const;
    void                         setPopupItem(QQuickItem*);
    QQuickItem*                  overlayItem() const;
    qreal                        overlayWidth() const;
    qreal                        overlayHeight() const;
    QQmlListProperty<QObject>    contentData();
    QQmlListProperty<QQuickItem> contentChildren();
    bool                         isVisible() const { return m_state != Closed; }
    void                         setVisible(bool);
    bool                         isOpened() const { return m_state == Open; }
    bool                         entering() const { return m_state == Entering; }
    bool                         closing() const { return m_state == Closing; }
    bool                         modal() const { return m_modal; }
    void                         setModal(bool);
    bool                         dim() const { return m_dim.value_or(m_modal); }
    void                         setDim(bool);
    void                         resetDim();
    bool                         focus() const { return m_focus; }
    void                         setFocus(bool);
    bool                         deferredCompletion() const { return m_deferred; }
    void                         setDeferredCompletion(bool);
    int                          hideDelay() const { return m_hideDelay; }
    void                         setHideDelay(int);
    ClosePolicy                  closePolicy() const { return m_closePolicy; }
    void                         setClosePolicy(ClosePolicy);
    CollisionPolicy              collisionPolicy() const { return m_collision; }
    void                         setCollisionPolicy(CollisionPolicy);
    qreal                        x() const { return m_position.x(); }
    qreal                        y() const { return m_position.y(); }
    qreal                        z() const { return m_z.value_or(m_inheritedZ); }
    void                         setX(qreal);
    void                         setY(qreal);
    void                         setZ(qreal);
    void                         resetZ();
    qreal                        margins() const { return m_margins; }
    void                         setMargins(qreal);
    qreal                        leftMargin() const { return m_leftMargin.value_or(m_margins); }
    void                         setLeftMargin(qreal);
    void                         resetLeftMargin();
    qreal                        rightMargin() const { return m_rightMargin.value_or(m_margins); }
    void                         setRightMargin(qreal);
    void                         resetRightMargin();
    qreal                        topMargin() const { return m_topMargin.value_or(m_margins); }
    void                         setTopMargin(qreal);
    void                         resetTopMargin();
    qreal                        bottomMargin() const { return m_bottomMargin.value_or(m_margins); }
    void                         setBottomMargin(qreal);
    void                         resetBottomMargin();
    QFont                        font() const;
    void                         setFont(const QFont&);
    void                         resetFont();
    QLocale                      locale() const;
    void                         setLocale(const QLocale&);
    void                         resetLocale();
    QQuickTransition*            enter() const;
    QQuickTransition*            exit() const;
    void                         setEnter(QQuickTransition*);
    void                         setExit(QQuickTransition*);
    QQuickItem*                  contentItem() const;
    void                         setContentItem(QQuickItem* value);
    Q_SIGNAL void                contentItemChanged();
    QQuickItem*                  background() const;
    void                         setBackground(QQuickItem* value);
    Q_SIGNAL void                backgroundChanged();
    qreal                        width() const { return m_width.value_or(implicitWidth()); }
    void                         setWidth(qreal value);
    void                         resetWidth();
    Q_SIGNAL void                widthChanged();
    qreal                        height() const { return m_height.value_or(implicitHeight()); }
    void                         setHeight(qreal value);
    void                         resetHeight();
    Q_SIGNAL void                heightChanged();
    qreal                        implicitWidth() const;
    void                         setImplicitWidth(qreal value);
    Q_SIGNAL void                implicitWidthChanged();
    qreal                        implicitHeight() const;
    void                         setImplicitHeight(qreal value);
    Q_SIGNAL void                implicitHeightChanged();
    qreal                        contentWidth() const;
    void                         setContentWidth(qreal value);
    void                         resetContentWidth();
    Q_SIGNAL void                contentWidthChanged();
    qreal                        contentHeight() const;
    void                         setContentHeight(qreal value);
    void                         resetContentHeight();
    Q_SIGNAL void                contentHeightChanged();
    qreal                        padding() const;
    void                         setPadding(qreal value);
    Q_SIGNAL void                paddingChanged();
    qreal                        horizontalPadding() const;
    void                         setHorizontalPadding(qreal value);
    Q_SIGNAL void                horizontalPaddingChanged();
    qreal                        verticalPadding() const;
    void                         setVerticalPadding(qreal value);
    Q_SIGNAL void                verticalPaddingChanged();
    qreal                        leftPadding() const;
    void                         setLeftPadding(qreal value);
    Q_SIGNAL void                leftPaddingChanged();
    qreal                        rightPadding() const;
    void                         setRightPadding(qreal value);
    Q_SIGNAL void                rightPaddingChanged();
    qreal                        topPadding() const;
    void                         setTopPadding(qreal value);
    Q_SIGNAL void                topPaddingChanged();
    qreal                        bottomPadding() const;
    void                         setBottomPadding(qreal value);
    Q_SIGNAL void                bottomPaddingChanged();
    qreal                        spacing() const;
    void                         setSpacing(qreal value);
    Q_SIGNAL void                spacingChanged();
    qreal                        leftInset() const;
    void                         setLeftInset(qreal value);
    Q_SIGNAL void                leftInsetChanged();
    qreal                        rightInset() const;
    void                         setRightInset(qreal value);
    Q_SIGNAL void                rightInsetChanged();
    qreal                        topInset() const;
    void                         setTopInset(qreal value);
    Q_SIGNAL void                topInsetChanged();
    qreal                        bottomInset() const;
    void                         setBottomInset(qreal value);
    Q_SIGNAL void                bottomInsetChanged();
    qreal                        opacity() const;
    void                         setOpacity(qreal value);
    Q_SIGNAL void                opacityChanged();
    qreal                        scale() const;
    void                         setScale(qreal value);
    Q_SIGNAL void                scaleChanged();
    QQuickItem::TransformOrigin  transformOrigin() const;
    void                         setTransformOrigin(QQuickItem::TransformOrigin value);
    Q_SIGNAL void                transformOriginChanged();
    bool                         enabled() const;
    void                         setEnabled(bool value);
    Q_SIGNAL void                enabledChanged();
    qreal                        availableWidth() const;
    Q_SIGNAL void                availableWidthChanged();
    qreal                        availableHeight() const;
    Q_SIGNAL void                availableHeightChanged();
    qreal                        implicitContentWidth() const;
    Q_SIGNAL void                implicitContentWidthChanged();
    qreal                        implicitContentHeight() const;
    Q_SIGNAL void                implicitContentHeightChanged();
    qreal                        implicitBackgroundWidth() const;
    Q_SIGNAL void                implicitBackgroundWidthChanged();
    qreal                        implicitBackgroundHeight() const;
    Q_SIGNAL void                implicitBackgroundHeightChanged();
    bool                         activeFocus() const;
    Q_SIGNAL void                activeFocusChanged();
    bool                         visualFocus() const;
    Q_SIGNAL void                visualFocusChanged();
    bool                         mirrored() const;
    Q_SIGNAL void                mirroredChanged();
    Q_INVOKABLE virtual void     open();
    Q_INVOKABLE virtual void     close();
    virtual void                 closeFromInput();
    Q_INVOKABLE void             completeEnter();
    Q_INVOKABLE void             completeExit();
    Q_INVOKABLE void             forceActiveFocus(Qt::FocusReason reason = Qt::OtherFocusReason);
    Q_INVOKABLE virtual void     dismissImmediately();
    bool                         acquirePresentation(QObject* owner);
    void                         releasePresentation(QObject* owner);
    void                         setPresentationAllowed(QObject* owner, bool allowed);
    void                         setPresentationRequestEnabled(QObject* owner, bool enabled);
    void                         setPresentationCloseHandler(QObject* owner, std::function<void()>);
    QRectF                       presentationRect() const;
    Q_SIGNAL void                presentationRectChanged();
    bool                         requestPresentation();
    bool                         canRequestPresentation() const {
        return presentationAllowed() || m_presentationRequestEnabled;
    }
    QObject* presentationOwner() const { return m_presentationOwner; }
    bool     presentationAllowed() const { return ! m_presentationOwner || m_presentationAllowed; }
    Q_SIGNAL void presentationOwnerChanged();
    Q_SIGNAL void presentationRequested();
    void          reposition();
    virtual void  updateDimmer(QQuickItem*, qreal opacity) const;
    bool          containsScenePoint(const QPointF&) const;
    virtual bool  blocksScenePoint(const QPointF&) const { return modal(); }
    virtual bool  overlayContainsScenePoint(const QPointF&) const { return true; }
    bool          parentContainsScenePoint(const QPointF&) const;
    Q_SIGNAL void parentChanged();
    Q_SIGNAL void popupItemChanged();
    Q_SIGNAL void overlayItemChanged();
    Q_SIGNAL void overlayGeometryChanged();
    Q_SIGNAL void contentChildrenChanged();
    Q_SIGNAL void visibleChanged();
    Q_SIGNAL void openedChanged();
    Q_SIGNAL void enteringChanged();
    Q_SIGNAL void closingChanged();
    Q_SIGNAL void modalChanged();
    Q_SIGNAL void dimChanged();
    Q_SIGNAL void focusChanged();
    Q_SIGNAL void positioningItemChanged();
    Q_SIGNAL void deferredCompletionChanged();
    Q_SIGNAL void hideDelayChanged();
    Q_SIGNAL void closePolicyChanged();
    Q_SIGNAL void collisionPolicyChanged();
    Q_SIGNAL void xChanged();
    Q_SIGNAL void yChanged();
    Q_SIGNAL void zChanged();
    Q_SIGNAL void marginsChanged();
    Q_SIGNAL void fontChanged();
    Q_SIGNAL void localeChanged();
    Q_SIGNAL void enterChanged();
    Q_SIGNAL void exitChanged();
    Q_SIGNAL void aboutToShow();
    Q_SIGNAL void aboutToHide();
    Q_SIGNAL void opened();
    Q_SIGNAL void closed();

protected:
    void beginInteractiveTransition();
    void endInteractiveTransition(bool opening);
    struct TransitionTarget {
        QString  property;
        QVariant value;
    };
    virtual QPointF                 surfacePosition() const;
    virtual QSizeF                  surfaceSize() const { return { width(), height() }; }
    virtual qreal                   presentationScale() const { return 1; }
    virtual QList<TransitionTarget> transitionTargets(bool) const { return {}; }
    virtual void                    finalizeTransition(bool) {}
    virtual bool                    inheritsHoverEnabled() const { return true; }
    Popup(Panel* surface, QObject* parent);
    void classBegin() override;
    void componentComplete() override;

private:
    friend class OverlayManager;
    void overlayPress(const QPointF&);
    void overlayRelease(const QPointF&);
    void cancelOverlayPress();
    bool m_outsidePressed = false, m_outsideParentPressed = false;
    enum State
    {
        Closed,
        Entering,
        Open,
        Closing
    };
    bool                           changeState(State);
    void                           observeParent();
    void                           observePositioningItem();
    void                           refreshEnvironment();
    void                           resolveFont();
    void                           resolveLocale();
    void                           resolveLayoutDirection();
    void                           resolveHoverEnabled();
    void                           refreshInheritedZ();
    void                           updateOverlay();
    void                           finishClose();
    void                           startTransition(bool);
    Panel*                         m_surface;
    QPointer<QQuickItem>           m_parent, m_hitItem, m_positioningItem;
    QPointer<Control>              m_environmentParent;
    QPointer<OverlayManager>       m_overlay;
    QList<QMetaObject::Connection> m_parentConnections, m_overlayConnections;
    QList<QMetaObject::Connection> m_positioningConnections;
    QPointer<QQuickTransition>     m_enter, m_exit;
    std::unique_ptr<PopupMotion>   m_motion;
    State                          m_state          = Closed;
    bool                           m_dismissing     = false;
    bool                           m_finishingClose = false;
    std::function<void()>          m_presentationCloseHandler;
    QQuickTransform*               m_presentationTransform = nullptr;
    std::optional<qreal>           m_width, m_height;
    bool                           m_interacting = false;
    QPointer<QObject>              m_presentationOwner;
    QMetaObject::Connection        m_presentationOwnerConnection;
    bool                           m_presentationAllowed = false;
    bool m_presentationRequestEnabled = false, m_requestingPresentation = false;
    bool m_complete = false, m_requestedVisible = false, m_modal = false, m_focus = false,
         m_deferred = false, m_positioning = false;
    std::optional<bool>    m_dim;
    ClosePolicy            m_closePolicy { CloseOnEscape | CloseOnPressOutside };
    CollisionPolicy        m_collision = Clamp;
    QPointF                m_position;
    std::optional<qreal>   m_z;
    qreal                  m_inheritedZ = 0, m_margins = 0;
    qreal                  m_savedOpacity = 1, m_savedScale = 1;
    std::optional<qreal>   m_leftMargin, m_rightMargin, m_topMargin, m_bottomMargin;
    QFont                  m_font;
    std::optional<QLocale> m_locale;
    int                    m_hideDelay = 0;
    QTimer                 m_hideTimer;
};
} // namespace qml_material
Q_DECLARE_OPERATORS_FOR_FLAGS(qml_material::Popup::ClosePolicy)
