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
                 return static_cast<ActionGroup*>(p->data)->m_selection.members.size();
             },
             [](QQmlListProperty<Action>* p, qsizetype i) {
                 return static_cast<ActionGroup*>(p->data)->m_selection.members.value(i);
             },
             [](QQmlListProperty<Action>* p) {
                 static_cast<ActionGroup*>(p->data)->clear();
             } };
}
void ActionGroup::clear() {
    QPointer<ActionGroup> guard(this);
    const auto            items = m_selection.snapshot();
    for (const auto& item : items) {
        if (item) removeAction(item);
        if (! guard) return;
    }
}
void ActionGroup::update(Action* preferred) {
    QPointer<ActionGroup> guard(this);
    m_selection.normalize(this, preferred);
    if (! guard) return;
    if (m_notified_selected != m_selection.selected) {
        m_notified_selected = m_selection.selected;
        Q_EMIT checkedActionChanged();
        if (! guard) return;
    }
}
void ActionGroup::setCheckedAction(Action* member) {
    if (! isExclusive() || (member && ! m_selection.members.contains(member))) return;
    QPointer<ActionGroup> guard(this);
    if (member) {
        QPointer<Action> item(member);
        member->setChecked(true);
        if (guard && item && item->group() == this) update(item);
    } else {
        const auto items = m_selection.snapshot();
        for (const auto& item : items) {
            if (item && item->group() == this) item->setChecked(false);
            if (! guard) return;
        }
        update();
    }
}
void ActionGroup::setExclusive(bool value) {
    if (isExclusive() == value) return;
    m_selection.exclusive = value;
    QPointer<ActionGroup> guard(this);
    update();
    if (guard) Q_EMIT exclusiveChanged();
}
void ActionGroup::addAction(Action* member) {
    if (! member || m_destroying || m_selection.members.contains(member)) return;
    QPointer<ActionGroup> guard(this);
    QPointer<Action>      item(member);
    if (member->group()) member->group()->removeAction(member);
    if (! guard || ! item || item->group()) return;
    const bool wasEnabled = item->isEnabled();
    m_selection.members.append(item);
    item->m_group     = this;
    auto& connections = m_connections[item];
    connections.append(connect(item, &Action::checkedChanged, this, [this, member]() {
        update(member);
    }));
    connections.append(connect(item, &Action::triggered, this, [this, member]() {
        Q_EMIT triggered(member);
    }));
    update(item);
    if (! guard || ! item || item->group() != this) return;
    Q_EMIT item->groupChanged();
    if (! guard || ! item || item->group() != this) return;
    if (wasEnabled != item->isEnabled()) Q_EMIT item->enabledChanged();
    if (! guard) return;
    Q_EMIT actionsChanged();
}
void ActionGroup::removeAction(Action* member) {
    if (! member || ! m_selection.members.removeOne(member)) return;
    QPointer<ActionGroup> guard(this);
    QPointer<Action>      item(member);
    const bool            wasEnabled  = item->isEnabled();
    auto                  connections = m_connections.take(member);
    utils::disconnectAll(connections);
    member->m_group = nullptr;
    update();
    if (! guard || ! item) return;
    Q_EMIT item->groupChanged();
    if (! guard || ! item) return;
    if (wasEnabled != item->isEnabled()) Q_EMIT item->enabledChanged();
    if (! guard) return;
    Q_EMIT actionsChanged();
}
void ActionGroup::setEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    QPointer<ActionGroup> guard(this);
    const auto            items = m_selection.snapshot();
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
