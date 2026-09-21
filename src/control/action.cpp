#include "qml_material/control/action.hpp"
#include "qml_material/control/action_group.hpp"

namespace qml_material
{
Action::Action(QObject* parent): QObject(parent), m_icon(new ActionIcon(this)) {}
Action::~Action() {
    if (m_group) m_group->removeAction(this);
}
void Action::setText(const QString& value) {
    if (m_text == value) return;
    m_text = value;
    Q_EMIT textChanged();
}
void Action::setVisible(bool value) {
    if (m_visible == value) return;
    m_visible = value;
    Q_EMIT visibleChanged();
}
void Action::setDisplayHint(int value) {
    if (m_displayHint == value) return;
    m_displayHint = value;
    Q_EMIT displayHintChanged();
}
void Action::setBusy(int value) {
    if (m_busy == value) return;
    m_busy = value;
    Q_EMIT busyChanged();
}
void Action::setProgress(qreal value) {
    if (m_progress == value) return;
    m_progress = value;
    Q_EMIT progressChanged();
}
void Action::setCloseMenu(bool value) {
    if (m_closeMenu == value) return;
    m_closeMenu = value;
    Q_EMIT closeMenuChanged();
}
void Action::setSeparator(bool value) {
    if (m_separator == value) return;
    m_separator = value;
    Q_EMIT separatorChanged();
}
void Action::setTooltip(const QString& value) {
    if (m_tooltip == value) return;
    m_tooltip = value;
    Q_EMIT tooltipChanged();
}
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
    if (m_checkable == value) return;
    m_checkable = value;
    Q_EMIT checkableChanged();
}
void Action::setChecked(bool value) {
    if (m_checked == value) return;
    m_checked = value;
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
