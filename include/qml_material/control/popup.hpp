#pragma once

#include <QQmlEngine>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>

class QFocusEvent;

namespace qml_material
{

class Popup : public QQuickItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(Popup2)

    Q_PROPERTY(bool opened READ isOpened NOTIFY openedChanged FINAL)
    Q_PROPERTY(bool entering READ entering NOTIFY enteringChanged FINAL)
    Q_PROPERTY(bool closing READ closing NOTIFY closingChanged FINAL)
    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged FINAL)
    Q_PROPERTY(bool dim READ dim WRITE setDim NOTIFY dimChanged FINAL)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setFullScreen NOTIFY fullScreenChanged FINAL)
    Q_PROPERTY(bool visualFocus READ visualFocus NOTIFY visualFocusChanged FINAL)
    Q_PROPERTY(Qt::FocusReason focusReason READ focusReason NOTIFY focusReasonChanged FINAL)
    Q_PROPERTY(ClosePolicy closePolicy READ closePolicy WRITE setClosePolicy NOTIFY
                   closePolicyChanged FINAL)
    Q_PROPERTY(bool deferredCompletion READ deferredCompletion WRITE setDeferredCompletion NOTIFY
                   deferredCompletionChanged FINAL)
    Q_PROPERTY(int hideDelay READ hideDelay WRITE setHideDelay NOTIFY hideDelayChanged FINAL)
    Q_PROPERTY(QQuickItem* popupItem READ popupItem WRITE setPopupItem NOTIFY popupItemChanged FINAL)
    Q_PROPERTY(QQuickItem* overlayItem READ overlayItem NOTIFY overlayItemChanged FINAL)
    Q_PROPERTY(qreal overlayWidth READ overlayWidth NOTIFY overlayGeometryChanged FINAL)
    Q_PROPERTY(qreal overlayHeight READ overlayHeight NOTIFY overlayGeometryChanged FINAL)

public:
    explicit Popup(QQuickItem* parent = nullptr);

    enum ClosePolicyFlag {
        NoAutoClose                 = 0x00,
        CloseOnPressOutside         = 0x01,
        CloseOnPressOutsideParent   = 0x02,
        CloseOnReleaseOutside       = 0x04,
        CloseOnReleaseOutsideParent = 0x08,
        CloseOnEscape               = 0x10
    };
    Q_DECLARE_FLAGS(ClosePolicy, ClosePolicyFlag)
    Q_FLAG(ClosePolicy)

    bool isOpened() const;
    bool entering() const;
    bool closing() const;

    bool modal() const;
    void setModal(bool value);

    bool dim() const;
    void setDim(bool value);

    bool fullScreen() const;
    void setFullScreen(bool value);

    bool visualFocus() const;
    Qt::FocusReason focusReason() const;

    ClosePolicy closePolicy() const;
    void        setClosePolicy(ClosePolicy value);

    bool deferredCompletion() const;
    void setDeferredCompletion(bool value);

    int  hideDelay() const;
    void setHideDelay(int value);

    QQuickItem* popupItem() const;
    void        setPopupItem(QQuickItem* item);

    QQuickItem* overlayItem() const;
    qreal       overlayWidth() const;
    qreal       overlayHeight() const;

    Q_INVOKABLE void open();
    Q_INVOKABLE void close();
    Q_INVOKABLE void completeEnter();
    Q_INVOKABLE void completeExit();

    Q_SIGNAL void aboutToShow();
    Q_SIGNAL void enteringChanged();
    Q_SIGNAL void openedChanged();
    Q_SIGNAL void opened();
    Q_SIGNAL void aboutToHide();
    Q_SIGNAL void closingChanged();
    Q_SIGNAL void closed();
    Q_SIGNAL void modalChanged();
    Q_SIGNAL void dimChanged();
    Q_SIGNAL void fullScreenChanged();
    Q_SIGNAL void visualFocusChanged();
    Q_SIGNAL void focusReasonChanged();
    Q_SIGNAL void closePolicyChanged();
    Q_SIGNAL void deferredCompletionChanged();
    Q_SIGNAL void hideDelayChanged();
    Q_SIGNAL void popupItemChanged();
    Q_SIGNAL void overlayItemChanged();
    Q_SIGNAL void overlayGeometryChanged();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool childMouseEventFilter(QQuickItem* item, QEvent* event) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    void setEntering(bool value);
    void setOpened(bool value);
    void setClosing(bool value);
    void finishClose();
    void updateInputAcceptance();
    void saveFocus();
    void restoreFocus();
    void setFocusReason(Qt::FocusReason reason);
    bool isVisualFocusReason(Qt::FocusReason reason) const;
    void updateOverlayItem();
    void setOverlayItem(QQuickItem* item);
    void useOverlayParent();
    void restoreParentItem();
    bool containsPopup(const QPointF& point) const;
    bool containsParent(const QPointF& point) const;
    bool tryClose(const QPointF& point, ClosePolicy flags);
    bool handlePress(const QPointF& point, QEvent* event);
    bool handleRelease(const QPointF& point, QEvent* event);
    bool blockInput(const QPointF& point, QEvent* event) const;

private:
    bool                 m_opened { false };
    bool                 m_entering { false };
    bool                 m_closing { false };
    bool                 m_modal { false };
    bool                 m_dim { false };
    bool                 m_fullScreen { false };
    bool                 m_deferredCompletion { false };
    bool                 m_outsidePressed { false };
    bool                 m_outsideParentPressed { false };
    Qt::FocusReason      m_focusReason { Qt::OtherFocusReason };
    ClosePolicy          m_closePolicy { CloseOnEscape | CloseOnPressOutside };
    int                  m_hideDelay { 0 };
    QPointer<QQuickItem> m_popupItem;
    QPointer<QQuickItem> m_overlayItem;
    QPointer<QQuickItem> m_previousFocusItem;
    QPointer<QQuickItem> m_previousParentItem;
    QPointF              m_previousPosition;
    QPointF              m_pressPoint;
    bool                 m_parentItemStored { false };
    QTimer               m_hideTimer;
};

} // namespace qml_material

Q_DECLARE_OPERATORS_FOR_FLAGS(qml_material::Popup::ClosePolicy)
