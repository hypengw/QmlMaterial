#include "qml_material/control/action.hpp"
#include <QProperty>
#include "qml_material/control/action_group.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
Action::Action(QObject* parent): QObject(parent), m_icon(new ActionIcon(this)) {
    m_checkableNotifier = m_checkable.addNotifier([this] {
        Q_EMIT checkableChanged();
    });
}
Action::~Action() {
    m_destroying = true;
    if (m_group) m_group->removeAction(this);
}
void Action::setText(const QString& value) { m_text = value; }
void Action::setVisible(bool value) { m_visible = value; }
void Action::setDisplayHint(int value) { m_displayHint = value; }
void Action::setBusy(int value) { m_busy = value; }
void Action::setProgress(qreal value) { m_progress = value; }
void Action::setCloseMenu(bool value) { m_closeMenu = value; }
void Action::setSeparator(bool value) { m_separator = value; }
void Action::setTooltip(const QString& value) { m_tooltip = value; }
void Action::setDisplayComponent(QQmlComponent* value) {
    if (m_displayComponent == value) return;
    disconnect(m_displayComponentConnection);
    m_displayComponent = value;
    if (value) {
        m_displayComponentConnection = connect(value, &QObject::destroyed, this, [this] {
            m_displayComponent = nullptr;
            Q_EMIT displayComponentChanged();
        });
    }
    Q_EMIT displayComponentChanged();
}
void Action::appendData(QObject* object) {
    m_data.append(object);
    if (object) {
        connect(object, &QObject::destroyed, this, [this, object] {
            bool changed = false;
            for (auto& entry : m_data) {
                if (entry == object) {
                    entry   = nullptr;
                    changed = true;
                }
            }
            if (changed) Q_EMIT dataChanged();
        });
    }
    Q_EMIT dataChanged();
}
QQmlListProperty<QObject> Action::data() {
    return { this,
             this,
             [](QQmlListProperty<QObject>* p, QObject* object) {
                 static_cast<Action*>(p->data)->appendData(object);
             },
             [](QQmlListProperty<QObject>* p) -> qsizetype {
                 return static_cast<Action*>(p->data)->m_data.size();
             },
             [](QQmlListProperty<QObject>* p, qsizetype index) {
                 return static_cast<Action*>(p->data)->m_data.value(index);
             },
             [](QQmlListProperty<QObject>* p) {
                 auto action = static_cast<Action*>(p->data);
                 if (action->m_data.isEmpty()) return;
                 action->m_data.clear();
                 Q_EMIT action->dataChanged();
             } };
}
bool Action::isEnabled() const { return m_enabled && (! m_group || m_group->isEnabled()); }
void Action::setEnabled(bool value) {
    const bool old = isEnabled();
    m_enabled      = value;
    if (old != isEnabled()) Q_EMIT enabledChanged();
}
void Action::setCheckable(bool value) { utils::updateBoundValue(m_checkable, value); }
bool Action::isChecked() const { return m_group ? m_group->isChecked(this) : m_standaloneChecked; }
void Action::setChecked(bool value) {
    if (m_group) {
        m_group->setMemberChecked(this, value);
        return;
    }
    if (m_standaloneChecked == value) return;
    m_standaloneChecked = value;
    Q_EMIT checkedChanged();
}
ActionGroup* Action::group() const { return m_group; }
void         Action::setGroup(ActionGroup* value) {
    if (m_group == value) return;
    if (value)
        value->addAction(this);
    else if (m_group)
        m_group->removeAction(this);
}
bool Action::canToggle() const { return acceptsChecked(! isChecked()); }
bool Action::acceptsChecked(bool value) const {
    return isEnabled() && m_checkable &&
           (value || ! isChecked() || ! m_group || ! m_group->isExclusive());
}
void Action::toggle(QObject* source) {
    if (! canTrigger() || ! canToggle()) return;
    QPointer<Action>  guard(this);
    QPointer<QObject> sourceGuard(source);
    const bool        previous = isChecked();
    m_triggering               = true;
    setChecked(! previous);
    if (! guard) return;
    if (isChecked() != previous) Q_EMIT toggled(sourceGuard);
    if (guard) m_triggering = false;
}
bool Action::trigger(QObject* source) {
    return activate(canToggle() ? std::optional<bool>(! isChecked()) : std::nullopt, source);
}
void Action::requestChecked(bool checked, QObject* source) {
    if (! acceptsChecked(checked)) return;
    activate(checked, source);
}
bool Action::activate(std::optional<bool> checked, QObject* source) {
    if (! canTrigger()) return false;
    QPointer<Action>  guard(this);
    QPointer<QObject> sourceGuard(source);
    m_triggering        = true;
    const bool previous = isChecked();
    if (checked) setChecked(*checked);
    if (! guard) return false;
    if (isChecked() != previous) Q_EMIT toggled(sourceGuard);
    if (! guard) return false;
    const bool dispatched = isEnabled();
    if (dispatched) Q_EMIT triggered(sourceGuard);
    if (guard) m_triggering = false;
    return dispatched;
}
} // namespace qml_material
