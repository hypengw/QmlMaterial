#pragma once

#include "qml_material/control/page.hpp"

#include <QQmlListProperty>
#include <QtQuick/private/qquickwindowmodule_p.h>

namespace qml_material
{

class QML_MATERIAL_API ApplicationWindow : public QQuickWindowQmlImpl {
    Q_OBJECT
    QML_NAMED_ELEMENT(ApplicationWindowBase)
    Q_CLASSINFO("DefaultProperty", "contentData")
    Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(
        QQuickItem* background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQuickItem* menuBar READ menuBar WRITE setMenuBar NOTIFY menuBarChanged FINAL)
    Q_PROPERTY(QQuickItem* header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem* footer READ footer WRITE setFooter NOTIFY footerChanged FINAL)
    Q_PROPERTY(QQuickItem* activeFocusControl READ activeFocusControl NOTIFY
                   activeFocusControlChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(
        QLocale locale READ locale WRITE setLocale RESET resetLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY
                   topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY
                   leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding
                   NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET
                   resetBottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(Page* frameItem READ frameItem CONSTANT FINAL)
    Q_PROPERTY(Page* bodyItem READ bodyItem CONSTANT FINAL)
public:
    explicit ApplicationWindow(QWindow* parent = nullptr);

    QQuickItem*               contentItem() const;
    QQmlListProperty<QObject> contentData();

    QQuickItem* background() const;
    void        setBackground(QQuickItem* item);
    QQuickItem* menuBar() const;
    void        setMenuBar(QQuickItem* item);
    QQuickItem* header() const;
    void        setHeader(QQuickItem* item);
    QQuickItem* footer() const;
    void        setFooter(QQuickItem* item);

    QQuickItem* activeFocusControl() const;

    QFont font() const;
    void  setFont(const QFont& font);
    void  resetFont();

    QLocale locale() const;
    void    setLocale(const QLocale& locale);
    void    resetLocale();

    qreal topPadding() const;
    void  setTopPadding(qreal value);
    void  resetTopPadding();
    qreal leftPadding() const;
    void  setLeftPadding(qreal value);
    void  resetLeftPadding();
    qreal rightPadding() const;
    void  setRightPadding(qreal value);
    void  resetRightPadding();
    qreal bottomPadding() const;
    void  setBottomPadding(qreal value);
    void  resetBottomPadding();

    Page* frameItem() const { return m_frame; }
    Page* bodyItem() const { return m_body; }

    Q_SIGNAL void backgroundChanged();
    Q_SIGNAL void menuBarChanged();
    Q_SIGNAL void headerChanged();
    Q_SIGNAL void footerChanged();
    Q_SIGNAL void activeFocusControlChanged();
    Q_SIGNAL void fontChanged();
    Q_SIGNAL void localeChanged();
    Q_SIGNAL void topPaddingChanged();
    Q_SIGNAL void leftPaddingChanged();
    Q_SIGNAL void rightPaddingChanged();
    Q_SIGNAL void bottomPaddingChanged();

private:
    void relayout();
    void updateActiveFocusControl();

    Page*                m_frame { nullptr };
    Page*                m_body { nullptr };
    QPointer<QQuickItem> m_activeFocusControl;
};

} // namespace qml_material
