#pragma once

#include <QPointer>
#include <QProperty>
#include <QQmlComponent>
#include <QQmlListProperty>
#include "qml_material/control/icon_spec.hpp"

namespace qml_material
{
class ActionGroup;
class QML_MATERIAL_API Action : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Action)
    Q_CLASSINFO("DefaultProperty", "data")
    Q_PROPERTY(QQmlListProperty<QObject> data READ data NOTIFY dataChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged BINDABLE
                   bindableVisible FINAL)
    Q_PROPERTY(int displayHint READ displayHint WRITE setDisplayHint NOTIFY displayHintChanged
                   BINDABLE bindableDisplayHint FINAL)
    Q_PROPERTY(int busy READ busy WRITE setBusy NOTIFY busyChanged BINDABLE bindableBusy FINAL)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged BINDABLE
                   bindableProgress FINAL)
    Q_PROPERTY(bool closeMenu READ closeMenu WRITE setCloseMenu NOTIFY closeMenuChanged BINDABLE
                   bindableCloseMenu FINAL)
    Q_PROPERTY(bool separator READ isSeparator WRITE setSeparator NOTIFY separatorChanged BINDABLE
                   bindableSeparator FINAL)
    Q_PROPERTY(QString tooltip READ tooltip WRITE setTooltip NOTIFY tooltipChanged BINDABLE
                   bindableTooltip FINAL)
    Q_PROPERTY(QQmlComponent* displayComponent READ displayComponent WRITE setDisplayComponent
                   NOTIFY displayComponentChanged FINAL)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged BINDABLE bindableText FINAL)
    Q_PROPERTY(ActionIcon* icon READ icon CONSTANT FINAL)
    Q_PROPERTY(
        bool enabled READ isEnabled WRITE setEnabled RESET resetEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY checkableChanged BINDABLE
                   bindableCheckable FINAL)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged BINDABLE
                   bindableChecked FINAL)
public:
    explicit Action(QObject* parent = nullptr);
    QBindable<bool>    bindableVisible() { return QBindable<bool>(&m_visible); }
    QBindable<int>     bindableDisplayHint() { return QBindable<int>(&m_displayHint); }
    QBindable<int>     bindableBusy() { return QBindable<int>(&m_busy); }
    QBindable<qreal>   bindableProgress() { return QBindable<qreal>(&m_progress); }
    QBindable<bool>    bindableCloseMenu() { return QBindable<bool>(&m_closeMenu); }
    QBindable<bool>    bindableSeparator() { return QBindable<bool>(&m_separator); }
    QBindable<QString> bindableTooltip() { return QBindable<QString>(&m_tooltip); }
    QBindable<QString> bindableText() { return QBindable<QString>(&m_text); }
    QBindable<bool>    bindableCheckable() { return QBindable<bool>(&m_checkable); }
    QBindable<bool>    bindableChecked() { return QBindable<bool>(&m_checked); }
    ~Action() override;
    QQmlListProperty<QObject> data();
    bool                      isVisible() const { return m_visible; }
    void                      setVisible(bool);
    Q_SIGNAL void             visibleChanged();
    int                       displayHint() const { return m_displayHint; }
    void                      setDisplayHint(int);
    Q_SIGNAL void             displayHintChanged();
    int                       busy() const { return m_busy; }
    void                      setBusy(int);
    Q_SIGNAL void             busyChanged();
    qreal                     progress() const { return m_progress; }
    void                      setProgress(qreal);
    Q_SIGNAL void             progressChanged();
    bool                      closeMenu() const { return m_closeMenu; }
    void                      setCloseMenu(bool);
    Q_SIGNAL void             closeMenuChanged();
    bool                      isSeparator() const { return m_separator; }
    void                      setSeparator(bool);
    Q_SIGNAL void             separatorChanged();
    QString                   tooltip() const { return m_tooltip; }
    void                      setTooltip(const QString&);
    Q_SIGNAL void             tooltipChanged();
    QQmlComponent*            displayComponent() const { return m_displayComponent; }
    void                      setDisplayComponent(QQmlComponent*);
    Q_SIGNAL void             displayComponentChanged();
    Q_SIGNAL void             dataChanged();
    QString                   text() const { return m_text; }
    void                      setText(const QString&);
    ActionIcon*               icon() const { return m_icon; }
    bool                      isEnabled() const;
    void                      setEnabled(bool);
    void                      resetEnabled() { setEnabled(true); }
    bool                      isCheckable() const { return m_checkable; }
    void                      setCheckable(bool);
    bool                      isChecked() const { return m_checked; }
    void                      setChecked(bool);
    ActionGroup*              group() const;
    void                      setGroup(ActionGroup*);
    bool                      canToggle() const;
    bool                      canTrigger() const { return isEnabled() && ! m_triggering; }
    Q_INVOKABLE void          toggle(QObject* source = nullptr);
    Q_INVOKABLE void          trigger(QObject* source = nullptr);
    // Activates a presentation that has already applied its checked transition.
    void          triggerFromControl(QObject* source, bool toggled);
    Q_SIGNAL void textChanged();
    Q_SIGNAL void enabledChanged();
    Q_SIGNAL void checkableChanged();
    Q_SIGNAL void checkedChanged();
    Q_SIGNAL void groupChanged();
    Q_SIGNAL void toggled(QObject* source);
    Q_SIGNAL void triggered(QObject* source);

private:
    friend class ActionGroup;
    void            checkedChange();
    void            appendData(QObject*);
    QList<QObject*> m_data;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, bool, m_visible, true, &Action::visibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, int, m_displayHint, 0, &Action::displayHintChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, int, m_busy, 0, &Action::busyChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, qreal, m_progress, 0, &Action::progressChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, bool, m_closeMenu, true, &Action::closeMenuChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, bool, m_separator, false,
                                         &Action::separatorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Action, QString, m_tooltip, &Action::tooltipChanged)
    QPointer<QQmlComponent> m_displayComponent;
    QMetaObject::Connection m_displayComponentConnection;
    Q_OBJECT_BINDABLE_PROPERTY(Action, QString, m_text, &Action::textChanged)
    ActionIcon*           m_icon;
    QPointer<ActionGroup> m_group;
    bool                  m_enabled = true;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, bool, m_checkable, false,
                                         &Action::checkableChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Action, bool, m_checked, false, &Action::checkedChange)
    bool m_triggering = false;
};
} // namespace qml_material
