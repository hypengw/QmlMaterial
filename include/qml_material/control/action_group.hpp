#pragma once

#include <QHash>
#include <QSet>
#include <QQmlListProperty>
#include <qqml.h>
#include "qml_material/control/action.hpp"

namespace qml_material
{
class ActionGroupAttached;
class QML_MATERIAL_API ActionGroup : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ActionGroup)
    QML_ATTACHED(ActionGroupAttached)
    Q_CLASSINFO("DefaultProperty", "actions")
    Q_PROPERTY(QQmlListProperty<Action> actions READ actions NOTIFY actionsChanged FINAL)
    Q_PROPERTY(Action* checkedAction READ checkedAction WRITE setCheckedAction NOTIFY
                   checkedActionChanged FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
public:
    explicit ActionGroup(QObject* parent = nullptr): QObject(parent) {}
    ~ActionGroup() override;
    static ActionGroupAttached* qmlAttachedProperties(QObject*);
    QQmlListProperty<Action>    actions();
    Action*                     checkedAction() const;
    void                        setCheckedAction(Action*);
    bool                        isExclusive() const { return m_exclusive; }
    void                        setExclusive(bool);
    bool                        isEnabled() const { return m_enabled; }
    void                        setEnabled(bool);
    Q_INVOKABLE void            addAction(Action*);
    Q_INVOKABLE void            removeAction(Action*);
    Q_SIGNAL void               actionsChanged();
    Q_SIGNAL void               checkedActionChanged();
    Q_SIGNAL void               exclusiveChanged();
    Q_SIGNAL void               enabledChanged();
    Q_SIGNAL void               triggered(Action* action);

private:
    friend class Action;
    bool                                           isChecked(const Action*) const;
    void                                           setMemberChecked(Action*, bool);
    void                                           notifySelection();
    void                                           clear();
    QList<QPointer<Action>>                        m_members;
    QSet<const Action*>                            m_checked;
    QHash<Action*, bool>                           m_notifiedChecked;
    QHash<Action*, QList<QMetaObject::Connection>> m_connections;
    Action*                                        m_notified_selected = nullptr;
    bool                                           m_exclusive         = true;
    bool                                           m_notifying         = false;
    bool                                           m_selectionDirty    = false;
    bool                                           m_enabled           = true;
    bool                                           m_destroying        = false;
};

class QML_MATERIAL_API ActionGroupAttached : public QObject {
    Q_OBJECT
    Q_PROPERTY(ActionGroup* group READ group WRITE setGroup NOTIFY groupChanged FINAL)
public:
    explicit ActionGroupAttached(QObject*);
    ActionGroup*  group() const;
    void          setGroup(ActionGroup*);
    Q_SIGNAL void groupChanged();

private:
    QPointer<Action> m_member;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::ActionGroup, QML_HAS_ATTACHED_PROPERTIES)
