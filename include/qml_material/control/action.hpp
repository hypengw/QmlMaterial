#pragma once

#include <QPointer>
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
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(
        int displayHint READ displayHint WRITE setDisplayHint NOTIFY displayHintChanged FINAL)
    Q_PROPERTY(int busy READ busy WRITE setBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(bool closeMenu READ closeMenu WRITE setCloseMenu NOTIFY closeMenuChanged FINAL)
    Q_PROPERTY(bool separator READ isSeparator WRITE setSeparator NOTIFY separatorChanged FINAL)
    Q_PROPERTY(QString tooltip READ tooltip WRITE setTooltip NOTIFY tooltipChanged FINAL)
    Q_PROPERTY(QQmlComponent* displayComponent READ displayComponent WRITE setDisplayComponent
                   NOTIFY displayComponentChanged FINAL)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(ActionIcon* icon READ icon CONSTANT FINAL)
    Q_PROPERTY(
        bool enabled READ isEnabled WRITE setEnabled RESET resetEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY checkableChanged FINAL)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
public:
    explicit Action(QObject* parent = nullptr);
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
    void                    appendData(QObject*);
    QList<QObject*>         m_data;
    bool                    m_visible     = true;
    int                     m_displayHint = 0;
    int                     m_busy        = 0;
    qreal                   m_progress    = 0;
    bool                    m_closeMenu   = true;
    bool                    m_separator   = false;
    QString                 m_tooltip;
    QPointer<QQmlComponent> m_displayComponent;
    QMetaObject::Connection m_displayComponentConnection;
    QString                 m_text;
    ActionIcon*             m_icon;
    QPointer<ActionGroup>   m_group;
    bool                    m_enabled    = true;
    bool                    m_checkable  = false;
    bool                    m_checked    = false;
    bool                    m_triggering = false;
};
} // namespace qml_material
