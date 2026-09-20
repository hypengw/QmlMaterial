#pragma once

#include <QTimer>
#include "qml_material/control/control.hpp"
#include "qml_material/control/icon_spec.hpp"
#include "qml_material/control/action.hpp"

namespace qml_material
{
class ButtonGroup;
class QML_MATERIAL_API AbstractButton : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractButtonBase)
    Q_PROPERTY(QString text READ text WRITE setText RESET resetText NOTIFY textChanged FINAL)
    Q_PROPERTY(IconSpec* icon READ icon CONSTANT FINAL)
    Q_PROPERTY(
        QQuickItem* indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(qreal implicitIndicatorWidth READ implicitIndicatorWidth NOTIFY
                   implicitIndicatorWidthChanged FINAL)
    Q_PROPERTY(qreal implicitIndicatorHeight READ implicitIndicatorHeight NOTIFY
                   implicitIndicatorHeightChanged FINAL)
    Q_PROPERTY(Action* action READ action WRITE setAction NOTIFY actionChanged FINAL)
    Q_PROPERTY(bool autoExclusive READ autoExclusive WRITE setAutoExclusive NOTIFY
                   autoExclusiveChanged FINAL)
    Q_PROPERTY(Display display READ display WRITE setDisplay NOTIFY displayChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool down READ isDown WRITE setDown RESET resetDown NOTIFY downChanged FINAL)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY checkableChanged FINAL)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(qreal pressX READ pressX NOTIFY pressXChanged FINAL)
    Q_PROPERTY(qreal pressY READ pressY NOTIFY pressYChanged FINAL)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat NOTIFY autoRepeatChanged FINAL)
    Q_PROPERTY(int autoRepeatDelay READ autoRepeatDelay WRITE setAutoRepeatDelay NOTIFY
                   autoRepeatDelayChanged FINAL)
    Q_PROPERTY(int autoRepeatInterval READ autoRepeatInterval WRITE setAutoRepeatInterval NOTIFY
                   autoRepeatIntervalChanged FINAL)
    Q_PROPERTY(int longPressInterval READ longPressInterval WRITE setLongPressInterval NOTIFY
                   longPressIntervalChanged FINAL)
public:
    enum Display
    {
        IconOnly,
        TextOnly,
        TextBesideIcon,
        TextUnderIcon
    };
    Q_ENUM(Display)
    explicit AbstractButton(QQuickItem* parent = nullptr);
    ~AbstractButton() override;
    QString      text() const;
    void         setText(const QString& value);
    void         resetText();
    Action*      action() const;
    void         setAction(Action*);
    ButtonGroup* group() const;
    void         setGroup(ButtonGroup*);
    bool         autoExclusive() const { return m_auto_exclusive; }
    void         setAutoExclusive(bool);
    IconSpec*    icon() const { return m_icon; }
    QQuickItem*  indicator() const { return m_indicator; }
    void         setIndicator(QQuickItem*);
    qreal implicitIndicatorWidth() const { return m_indicator ? m_indicator->implicitWidth() : 0; }
    qreal implicitIndicatorHeight() const {
        return m_indicator ? m_indicator->implicitHeight() : 0;
    }
    Q_SIGNAL void    indicatorChanged();
    Q_SIGNAL void    implicitIndicatorWidthChanged();
    Q_SIGNAL void    implicitIndicatorHeightChanged();
    Display          display() const { return m_display; }
    void             setDisplay(Display value);
    bool             isPressed() const { return m_pressed; }
    bool             isDown() const { return m_down.value_or(m_pressed); }
    void             setDown(bool value);
    void             resetDown();
    bool             isCheckable() const { return m_checkable; }
    void             setCheckable(bool value);
    bool             isChecked() const { return m_checked; }
    void             setChecked(bool value);
    qreal            pressX() const { return m_point.x(); }
    qreal            pressY() const { return m_point.y(); }
    bool             autoRepeat() const { return m_auto_repeat; }
    void             setAutoRepeat(bool value);
    int              autoRepeatDelay() const { return m_repeat_delay; }
    void             setAutoRepeatDelay(int value);
    int              autoRepeatInterval() const { return m_repeat_interval; }
    void             setAutoRepeatInterval(int value);
    int              longPressInterval() const { return m_hold_interval; }
    void             setLongPressInterval(int value);
    Q_INVOKABLE void click();
    Q_INVOKABLE void animateClick();
    Q_INVOKABLE void toggle();
    Q_SIGNAL void    pressed();
    Q_SIGNAL void    released();
    Q_SIGNAL void    canceled();
    Q_SIGNAL void    clicked();
    Q_SIGNAL void    toggled();
    Q_SIGNAL void    pressAndHold();
    Q_SIGNAL void    doubleClicked();
    Q_SIGNAL void    textChanged();
    Q_SIGNAL void    actionChanged();
    Q_SIGNAL void    groupChanged();
    Q_SIGNAL void    autoExclusiveChanged();
    Q_SIGNAL void    displayChanged();
    Q_SIGNAL void    pressedChanged();
    Q_SIGNAL void    downChanged();
    Q_SIGNAL void    checkableChanged();
    Q_SIGNAL void    checkedChanged();
    Q_SIGNAL void    pressXChanged();
    Q_SIGNAL void    pressYChanged();
    Q_SIGNAL void    autoRepeatChanged();
    Q_SIGNAL void    autoRepeatDelayChanged();
    Q_SIGNAL void    autoRepeatIntervalChanged();
    Q_SIGNAL void    longPressIntervalChanged();

protected:
    virtual void nextCheckState();
    virtual void pointerStarted(const QPointF&) {}
    virtual void pointerMoved(const QPointF&) {}
    virtual void interactionEnded() {}
    virtual bool retainPressOutside() const { return false; }
    void         setCheckedByInteraction(bool);
    bool         canUncheck() const;
    void         mousePressEvent(QMouseEvent* event) override;
    void         mouseMoveEvent(QMouseEvent* event) override;
    void         mouseReleaseEvent(QMouseEvent* event) override;
    void         mouseDoubleClickEvent(QMouseEvent* event) override;
    void         mouseUngrabEvent() override;
    void         touchEvent(QTouchEvent* event) override;
    void         touchUngrabEvent() override;
    void         keyPressEvent(QKeyEvent* event) override;
    void         keyReleaseEvent(QKeyEvent* event) override;
    void         focusOutEvent(QFocusEvent* event) override;
    void         itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    friend class ButtonGroup;
    void enforceAutoExclusive();
    void activate(bool changed);
    enum class Input
    {
        None,
        Mouse,
        Touch,
        Key,
        Simulated
    };
    void begin(Input input, const QPointF& point);
    void move(const QPointF& point);
    void finish(const QPointF& point);
    void cancel();
    void changePressed(bool value);
    void changePoint(const QPointF& point);
    void startTimers();
    void stopTimers();
    void repeat();
    void observeWindow();
    bool canActivate() const;

    IconSpec*                      m_icon;
    QPointer<QQuickItem>           m_indicator;
    QList<QMetaObject::Connection> m_indicator_connections;
    QString                        m_text;
    bool                           m_explicit_text  = false;
    bool                           m_auto_exclusive = false;
    QPointer<Action>               m_action;
    QPointer<ButtonGroup>          m_group;
    QList<QMetaObject::Connection> m_action_connections;
    Display                        m_display = TextBesideIcon;
    std::optional<bool>            m_down;
    bool                           m_pressed         = false;
    bool                           m_checked         = false;
    bool                           m_checkable       = false;
    bool                           m_auto_repeat     = false;
    bool                           m_held            = false;
    bool                           m_repeated        = false;
    bool                           m_double_click    = false;
    int                            m_repeat_delay    = 300;
    int                            m_repeat_interval = 100;
    int                            m_hold_interval;
    int                            m_touch_id = -1;
    int                            m_key      = 0;
    quint64                        m_sequence = 0;
    Input                          m_input    = Input::None;
    QPointF                        m_point;
    QPointF                        m_origin;
    QTimer                         m_repeat_timer;
    QTimer                         m_hold_timer;
    QTimer                         m_animate_timer;
    QMetaObject::Connection        m_window_connection;
};
} // namespace qml_material
