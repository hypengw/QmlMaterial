/* SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * Adapted from KDE Kirigami; integrated into qml_material.
 */

#pragma once

#include <QGuiApplication>
#include <QObject>
#include <QPoint>
#include <QPropertyAnimation>
#include <QQmlParserStatus>
#include <QQuickItem>
#include <QStyleHints>
#include <QTimer>
#include <QQmlProperty>
#include <QQmlEngine>

class QWheelEvent;
class QQmlEngine;

namespace qml_material
{

class WheelHandler;

/**
 * Describes the mouse wheel event passed through to QML signal handlers.
 */
class KirigamiWheelEvent : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal x READ x CONSTANT FINAL)
    Q_PROPERTY(qreal y READ y CONSTANT FINAL)
    Q_PROPERTY(QPointF angleDelta READ angleDelta CONSTANT FINAL)
    Q_PROPERTY(QPointF pixelDelta READ pixelDelta CONSTANT FINAL)
    Q_PROPERTY(int buttons READ buttons CONSTANT FINAL)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT FINAL)
    Q_PROPERTY(bool inverted READ inverted CONSTANT FINAL)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted FINAL)

public:
    KirigamiWheelEvent(QObject* parent = nullptr);
    ~KirigamiWheelEvent() override;

    void initializeFromEvent(QWheelEvent* event);

    qreal   x() const;
    qreal   y() const;
    QPointF angleDelta() const;
    QPointF pixelDelta() const;
    int     buttons() const;
    int     modifiers() const;
    bool    inverted() const;
    bool    isAccepted();
    void    setAccepted(bool accepted);

private:
    qreal                 m_x = 0;
    qreal                 m_y = 0;
    QPointF               m_angleDelta;
    QPointF               m_pixelDelta;
    Qt::MouseButtons      m_buttons   = Qt::NoButton;
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;
    bool                  m_inverted  = false;
    bool                  m_accepted  = false;
};

class WheelFilterItem : public QQuickItem {
    Q_OBJECT
public:
    WheelFilterItem(QQuickItem* parent = nullptr);
};

/**
 * @brief Handles scrolling for a Flickable and 2 attached ScrollBars.
 */
class WheelHandler : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(qreal verticalStepSize READ verticalStepSize WRITE setVerticalStepSize RESET
                   resetVerticalStepSize NOTIFY verticalStepSizeChanged FINAL)
    Q_PROPERTY(qreal horizontalStepSize READ horizontalStepSize WRITE setHorizontalStepSize RESET
                   resetHorizontalStepSize NOTIFY horizontalStepSizeChanged FINAL)
    Q_PROPERTY(Qt::KeyboardModifiers pageScrollModifiers READ pageScrollModifiers WRITE
                   setPageScrollModifiers RESET resetPageScrollModifiers NOTIFY
                       pageScrollModifiersChanged FINAL)
    Q_PROPERTY(Qt::KeyboardModifiers horizontalScrollModifiers READ horizontalScrollModifiers WRITE
                   setHorizontalScrollModifiers RESET resetHorizontalScrollModifiers NOTIFY
                       horizontalScrollModifiersChanged FINAL)
    Q_PROPERTY(bool filterMouseEvents READ filterMouseEvents WRITE setFilterMouseEvents NOTIFY
                   filterMouseEventsChanged FINAL)
    Q_PROPERTY(bool keyNavigationEnabled READ keyNavigationEnabled WRITE setKeyNavigationEnabled
                   NOTIFY keyNavigationEnabledChanged FINAL)
    Q_PROPERTY(bool blockTargetWheel MEMBER m_blockTargetWheel NOTIFY blockTargetWheelChanged FINAL)
    Q_PROPERTY(bool scrollFlickableTarget MEMBER m_scrollFlickableTarget NOTIFY
                   scrollFlickableTargetChanged FINAL)
    Q_PROPERTY(
        bool useAnimation READ useAnimation WRITE setUseAnimation NOTIFY useAnimationChanged FINAL)

public:
    explicit WheelHandler(QObject* parent = nullptr);
    ~WheelHandler() override;

    QQuickItem* target() const;
    void        setTarget(QQuickItem* target);

    auto active() const -> bool;
    void setActive(bool);
    qreal verticalStepSize() const;
    void  setVerticalStepSize(qreal stepSize);
    void  resetVerticalStepSize();

    qreal horizontalStepSize() const;
    void  setHorizontalStepSize(qreal stepSize);
    void  resetHorizontalStepSize();

    Qt::KeyboardModifiers pageScrollModifiers() const;
    void                  setPageScrollModifiers(Qt::KeyboardModifiers modifiers);
    void                  resetPageScrollModifiers();

    Qt::KeyboardModifiers horizontalScrollModifiers() const;
    void                  setHorizontalScrollModifiers(Qt::KeyboardModifiers modifiers);
    void                  resetHorizontalScrollModifiers();

    bool filterMouseEvents() const;
    void setFilterMouseEvents(bool enabled);

    bool keyNavigationEnabled() const;
    void setKeyNavigationEnabled(bool enabled);

    bool useAnimation() const;
    void setUseAnimation(bool);

    Q_INVOKABLE bool scrollUp(qreal stepSize = -1);
    Q_INVOKABLE bool scrollDown(qreal stepSize = -1);

    Q_INVOKABLE bool scrollLeft(qreal stepSize = -1);
    Q_INVOKABLE bool scrollRight(qreal stepSize = -1);

Q_SIGNALS:
    void targetChanged();
    void activeChanged();
    void verticalStepSizeChanged();
    void horizontalStepSizeChanged();
    void pageScrollModifiersChanged();
    void horizontalScrollModifiersChanged();
    void filterMouseEventsChanged();
    void keyNavigationEnabledChanged();
    void blockTargetWheelChanged();
    void scrollFlickableTargetChanged();
    void useAnimationChanged();

    void wheel(KirigamiWheelEvent* wheel);
    void wheelMoved();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void refreshAttach();
    void attach();
    void detach();
    void rebindScrollBarV();
    void rebindScrollBarH();

private:
    struct ScrollBar;
    void rebindScrollBar(ScrollBar& scrollBar);
    void classBegin() override;
    void componentComplete() override;

    void setScrolling(bool scrolling);
    bool scrollFlickable(QPointF pixelDelta, QPointF angleDelta = {},
                         Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    QPointer<QQuickItem> m_target;

    bool m_active;
    QMetaObject::Connection m_verticalChangedConnection;
    QMetaObject::Connection m_horizontalChangedConnection;

    qreal m_defaultPixelStepSize = 20 * QGuiApplication::styleHints()->wheelScrollLines();
    qreal m_verticalStepSize     = m_defaultPixelStepSize;
    qreal m_horizontalStepSize   = m_defaultPixelStepSize;
    bool  m_explicitVStepSize    = false;
    bool  m_explicitHStepSize    = false;
    bool  m_wheelScrolling       = false;
    constexpr static qreal m_wheelScrollingDuration = 400;
    bool                   m_filterMouseEvents      = false;
    bool                   m_keyNavigationEnabled   = false;
    bool                   m_blockTargetWheel       = true;
    bool                   m_scrollFlickableTarget  = true;
    constexpr static Qt::KeyboardModifiers m_defaultHorizontalScrollModifiers = Qt::AltModifier;
    constexpr static Qt::KeyboardModifiers m_defaultPageScrollModifiers =
        Qt::ControlModifier | Qt::ShiftModifier;
    Qt::KeyboardModifiers m_pageScrollModifiers        = m_defaultPageScrollModifiers;
    Qt::KeyboardModifiers m_horizontalScrollModifiers = m_defaultHorizontalScrollModifiers;
    QTimer                m_wheelScrollingTimer;
    KirigamiWheelEvent    m_kirigamiWheelEvent;

    QQmlEngine* m_engine     = nullptr;
    bool        m_wasTouched = false;

    struct Flickable {
        QQmlProperty originX, originY;
        QQmlProperty leftMargin, rightMargin;
        QQmlProperty topMargin, bottomMargin;
        QQmlProperty contentX, contentY;
        QQmlProperty contentHeight, contentWidth;
        QQmlProperty height, width;
        QQmlProperty interactive;
    } m_flickable;

    struct ScrollBar {
        QQmlProperty scrollBar;
        QQmlProperty stepSize;
        QMetaMethod  increaseMethod;
        QMetaMethod  decreaseMethod;
        QQuickItem*  item = nullptr;
        bool         valid() const { return item != nullptr; };
    } m_scrollBarV, m_scrollBarH;

    bool m_useAnimation = false;
};

} // namespace qml_material
