#pragma once

#include "qml_material/control/action.hpp"
#include "qml_material/control/command_button.hpp"

namespace qml_material
{
class QML_MATERIAL_API ActionCommand : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ActionCommand)
    QML_UNCREATABLE("Attached to a command button")
    QML_ATTACHED(ActionCommand)
    Q_PROPERTY(Action* action READ action WRITE setAction NOTIFY actionChanged FINAL)
    Q_PROPERTY(QString text READ text NOTIFY textChanged FINAL)
    Q_PROPERTY(ActionIcon* icon READ icon NOTIFY actionChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged FINAL)
public:
    static ActionCommand* qmlAttachedProperties(QObject*);
    Action*               action() const { return m_action; }
    void                  setAction(Action*);
    QString               text() const { return m_action ? m_action->text() : QString(); }
    ActionIcon*           icon() const { return m_action ? m_action->icon() : nullptr; }
    bool                  isEnabled() const { return ! m_action || m_action->isEnabled(); }
    Q_SIGNAL void         actionChanged();
    Q_SIGNAL void         textChanged();
    Q_SIGNAL void         enabledChanged();

private:
    explicit ActionCommand(CommandButton*);
    QPointer<CommandButton>        m_button;
    QPointer<Action>               m_action;
    QList<QMetaObject::Connection> m_connections;
    quint64                        m_revision = 0;
};
} // namespace qml_material
QML_DECLARE_TYPEINFO(qml_material::ActionCommand, QML_HAS_ATTACHED_PROPERTIES)
