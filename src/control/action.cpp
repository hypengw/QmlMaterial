#include "qml_material/control/action.hpp"
#include <QProperty>
#include "qml_material/control/action_group.hpp"

namespace qml_material
{
Action::Action(QObject* parent): QObject(parent), m_icon(new ActionIcon(this)) {}
Action::~Action() {
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
void Action::setCheckable(bool value) {
    const QScopedPropertyUpdateGroup group;
    m_checkable = value;
}
void Action::setChecked(bool value) {
    const QScopedPropertyUpdateGroup group;
    m_checked = value;
}
void Action::checkedChange() {
    const QScopedPropertyUpdateGroup group;
    const QPointer<Action>           guard(this);
    if (m_group) m_group->update(this);
    if (guard) Q_EMIT checkedChanged();
}
ActionGroup* Action::group() const { return m_group; }
void         Action::setGroup(ActionGroup* value) {
    if (m_group == value) return;
    if (value)
        value->addAction(this);
    else if (m_group)
        m_group->removeAction(this);
}
bool Action::canToggle() const {
    return m_checkable && (! m_checked || ! m_group || ! m_group->isExclusive());
}
void Action::toggle(QObject* source) {
    if (! isEnabled() || ! canToggle()) return;
    QPointer<Action>  guard(this);
    QPointer<QObject> sourceGuard(source);
    setChecked(! m_checked);
    if (guard) Q_EMIT toggled(sourceGuard);
}
void Action::trigger(QObject* source) {
    if (! canTrigger()) return;
    QPointer<Action>  guard(this);
    QPointer<QObject> sourceGuard(source);
    m_triggering = true;
    toggle(source);
    if (! guard) return;
    if (isEnabled()) Q_EMIT triggered(sourceGuard);
    if (guard) m_triggering = false;
}
void Action::triggerFromControl(QObject* source, bool changed) {
    if (! canTrigger()) return;
    QPointer<Action>  guard(this);
    QPointer<QObject> sourceGuard(source);
    m_triggering = true;
    if (changed) Q_EMIT toggled(sourceGuard);
    if (! guard) return;
    if (isEnabled()) Q_EMIT triggered(sourceGuard);
    if (guard) m_triggering = false;
}
} // namespace qml_material
