#include "qml_material/control/action_group.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
ActionGroup::~ActionGroup() {
    m_destroying = true;
    clear();
}
ActionGroupAttached* ActionGroup::qmlAttachedProperties(QObject* object) {
    return new ActionGroupAttached(object);
}
QQmlListProperty<Action> ActionGroup::actions() {
    return { this,
             this,
             [](QQmlListProperty<Action>* p, Action* item) {
                 static_cast<ActionGroup*>(p->data)->addAction(item);
             },
             [](QQmlListProperty<Action>* p) -> qsizetype {
                 return static_cast<ActionGroup*>(p->data)->m_members.size();
             },
             [](QQmlListProperty<Action>* p, qsizetype i) -> Action* {
                 return static_cast<ActionGroup*>(p->data)->m_members.value(i);
             },
             [](QQmlListProperty<Action>* p) {
                 static_cast<ActionGroup*>(p->data)->clear();
             } };
}
void ActionGroup::clear() {
    QPointer<ActionGroup> guard(this);
    const auto            items = m_members;
    for (const auto& item : items) {
        if (item && item->group() == this) removeAction(item);
        if (! guard) return;
    }
}
bool    ActionGroup::isChecked(const Action* member) const { return m_checked.contains(member); }
Action* ActionGroup::checkedAction() const {
    if (! m_exclusive) return nullptr;
    for (const auto& member : m_members)
        if (member && isChecked(member)) return member;
    return nullptr;
}
void ActionGroup::setMemberChecked(Action* member, bool checked) {
    if (! member || member->group() != this) return;
    if (checked) {
        if (m_exclusive) m_checked.clear();
        m_checked.insert(member);
    } else {
        m_checked.remove(member);
    }
    notifySelection();
}
void ActionGroup::notifySelection() {
    if (m_destroying) return;
    m_selectionDirty = true;
    if (m_notifying) return;
    m_notifying = true;
    QPointer<ActionGroup> guard(this);
    // Commit the whole selection before notifying any member. Reentrant requests
    // are observed from the new selection, never from partially updated buttons.
    while (m_selectionDirty) {
        m_selectionDirty   = false;
        const auto members = m_members;
        for (const auto& member : members) {
            if (! member || member->group() != this) continue;
            const bool checked = isChecked(member);
            if (m_notifiedChecked.value(member) == checked) continue;
            m_notifiedChecked[member] = checked;
            Q_EMIT member->checkedChanged();
            if (! guard) return;
        }
        auto* selected = checkedAction();
        if (m_notified_selected != selected) {
            m_notified_selected = selected;
            Q_EMIT checkedActionChanged();
            if (! guard) return;
        }
    }
    m_notifying = false;
}
void ActionGroup::setCheckedAction(Action* member) {
    if (! isExclusive() || (member && member->group() != this)) return;
    m_checked.clear();
    if (member) m_checked.insert(member);
    notifySelection();
}
void ActionGroup::setExclusive(bool value) {
    if (isExclusive() == value) return;
    m_exclusive = value;
    if (value) {
        auto* selected = checkedAction();
        m_checked.clear();
        if (selected) m_checked.insert(selected);
    }
    QPointer<ActionGroup> guard(this);
    notifySelection();
    if (guard) Q_EMIT exclusiveChanged();
}
void ActionGroup::addAction(Action* member) {
    if (! member || m_destroying || member->m_destroying || m_members.contains(member)) return;
    QPointer<ActionGroup> guard(this);
    QPointer<Action>      item(member);
    if (member->group()) member->group()->removeAction(member);
    if (! guard || ! item || item->group()) return;
    const bool wasEnabled = item->isEnabled();
    const bool wasChecked = item->isChecked();
    m_members.append(item);
    m_notifiedChecked[item] = wasChecked;
    item->m_group           = this;
    auto& connections       = m_connections[item];
    connections.append(connect(item, &Action::triggered, this, [this, member]() {
        Q_EMIT triggered(member);
    }));
    setMemberChecked(item, wasChecked);
    if (! guard || ! item || item->group() != this) return;
    Q_EMIT item->groupChanged();
    if (! guard || ! item || item->group() != this) return;
    if (wasEnabled != item->isEnabled()) Q_EMIT item->enabledChanged();
    if (! guard) return;
    Q_EMIT actionsChanged();
}
void ActionGroup::removeAction(Action* member) {
    if (! member || member->group() != this) return;
    QPointer<ActionGroup> guard(this);
    QPointer<Action>      item(member);
    const bool            wasEnabled  = item->isEnabled();
    const bool            wasNotified = m_notifiedChecked.value(member);
    member->m_standaloneChecked       = isChecked(member);
    m_members.removeOne(member);
    m_checked.remove(member);
    m_notifiedChecked.remove(member);
    auto connections = m_connections.take(member);
    utils::disconnectAll(connections);
    member->m_group = nullptr;
    notifySelection();
    if (! guard) return;
    if (item && ! item->m_destroying) {
        if (wasNotified != item->isChecked()) Q_EMIT item->checkedChanged();
        if (! guard) return;
        if (item) Q_EMIT item->groupChanged();
        if (! guard) return;
        if (item && wasEnabled != item->isEnabled()) Q_EMIT item->enabledChanged();
    }
    if (! guard || m_destroying) return;
    Q_EMIT actionsChanged();
}
void ActionGroup::setEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    QPointer<ActionGroup> guard(this);
    const auto            items = m_members;
    for (const auto& item : items) {
        if (item && item->group() == this && item->m_enabled) Q_EMIT item->enabledChanged();
        if (! guard) return;
    }
    Q_EMIT enabledChanged();
}
ActionGroupAttached::ActionGroupAttached(QObject* parent)
    : QObject(parent), m_member(qobject_cast<Action*>(parent)) {
    if (m_member)
        connect(m_member, &Action::groupChanged, this, &ActionGroupAttached::groupChanged);
}
ActionGroup* ActionGroupAttached::group() const { return m_member ? m_member->group() : nullptr; }
void         ActionGroupAttached::setGroup(ActionGroup* value) {
    if (m_member) m_member->setGroup(value);
}
} // namespace qml_material
