#include "qml_material/control/action_command.hpp"
#include "qml_material/util/qt.hpp"
#include <QQmlInfo>

namespace qml_material
{
ActionCommand* ActionCommand::qmlAttachedProperties(QObject* object) {
    auto* button = qobject_cast<CommandButton*>(object);
    if (! button) {
        qmlWarning(object) << "ActionCommand requires a command button";
        return nullptr;
    }
    return new ActionCommand(button);
}
ActionCommand::ActionCommand(CommandButton* button): QObject(button), m_button(button) {}
void ActionCommand::setAction(Action* value) {
    // QPointer is already cleared when the action's destroyed signal is delivered.
    if (m_action == value && (value || m_connections.isEmpty())) return;
    utils::disconnectAll(m_connections);
    m_action            = value;
    const auto revision = ++m_revision;
    if (value) {
        m_connections.append(connect(m_button,
                                     &CommandButton::clicked,
                                     this,
                                     [button = m_button, action = QPointer<Action>(value)] {
                                         if (button && button->canActivate() && action)
                                             action->trigger(button);
                                     }));
        m_connections.append(
            connect(value, &Action::textChanged, this, &ActionCommand::textChanged));
        m_connections.append(connect(value, &Action::enabledChanged, this, [this] {
            const QPointer<ActionCommand> guard(this);
            const auto                    revision = m_revision;
            if (! isEnabled() && m_button) m_button->cancelActivation();
            if (guard && m_revision == revision) Q_EMIT enabledChanged();
        }));
        m_connections.append(connect(value, &QObject::destroyed, this, [this] {
            setAction(nullptr);
        }));
    }
    const QPointer<ActionCommand> guard(this);
    if (m_button) m_button->cancelActivation();
    if (! guard || m_revision != revision) return;
    Q_EMIT actionChanged();
    if (! guard || m_revision != revision) return;
    Q_EMIT textChanged();
    if (guard && m_revision == revision) Q_EMIT enabledChanged();
}
} // namespace qml_material
