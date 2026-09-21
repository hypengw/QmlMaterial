#include "qml_material/control/button_group.hpp"
#include "qml_material/util/qt.hpp"

namespace qml_material
{
ButtonGroup::~ButtonGroup() {
    m_destroying = true;
    clear();
}
ButtonGroupAttached* ButtonGroup::qmlAttachedProperties(QObject* object) {
    return new ButtonGroupAttached(object);
}
QQmlListProperty<AbstractButton> ButtonGroup::buttons() {
    return { this,
             this,
             [](QQmlListProperty<AbstractButton>* p, AbstractButton* item) {
                 static_cast<ButtonGroup*>(p->data)->addButton(item);
             },
             [](QQmlListProperty<AbstractButton>* p) -> qsizetype {
                 return static_cast<ButtonGroup*>(p->data)->m_selection.members.size();
             },
             [](QQmlListProperty<AbstractButton>* p, qsizetype i) {
                 return static_cast<ButtonGroup*>(p->data)->m_selection.members.value(i);
             },
             [](QQmlListProperty<AbstractButton>* p) {
                 static_cast<ButtonGroup*>(p->data)->clear();
             } };
}
void ButtonGroup::clear() {
    QPointer<ButtonGroup> guard(this);
    const auto            items = m_selection.snapshot();
    for (const auto& item : items) {
        if (item) removeButton(item);
        if (! guard) return;
    }
}
void ButtonGroup::update(AbstractButton* preferred) {
    QPointer<ButtonGroup> guard(this);
    m_selection.normalize(this, preferred);
    if (! guard) return;
    if (m_notified_selected != m_selection.selected) {
        m_notified_selected = m_selection.selected;
        Q_EMIT checkedButtonChanged();
        if (! guard) return;
    }
    const auto state = m_selection.checkState();
    if (m_check_state != state) {
        m_check_state = state;
        Q_EMIT checkStateChanged();
    }
}
void ButtonGroup::setCheckedButton(AbstractButton* member) {
    if (! isExclusive() || (member && ! m_selection.members.contains(member))) return;
    QPointer<ButtonGroup> guard(this);
    if (member) {
        QPointer<AbstractButton> item(member);
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
void ButtonGroup::setExclusive(bool value) {
    if (isExclusive() == value) return;
    m_selection.exclusive = value;
    QPointer<ButtonGroup> guard(this);
    update();
    if (guard) Q_EMIT exclusiveChanged();
}
void ButtonGroup::addButton(AbstractButton* member) {
    if (! member || m_destroying || m_selection.members.contains(member)) return;
    QPointer<ButtonGroup>    guard(this);
    QPointer<AbstractButton> item(member);
    if (member->group()) member->group()->removeButton(member);
    if (! guard || ! item || item->group()) return;

    m_selection.members.append(item);
    item->m_group     = this;
    auto& connections = m_connections[item];
    connections.append(connect(item, &AbstractButton::checkedChanged, this, [this, member]() {
        update(member);
    }));
    connections.append(connect(item, &AbstractButton::clicked, this, [this, member]() {
        Q_EMIT clicked(member);
    }));
    update(item);
    if (! guard || ! item || item->group() != this) return;
    Q_EMIT item->groupChanged();
    if (! guard || ! item || item->group() != this) return;

    Q_EMIT buttonsChanged();
}
void ButtonGroup::removeButton(AbstractButton* member) {
    if (! member || ! m_selection.members.removeOne(member)) return;
    QPointer<ButtonGroup>    guard(this);
    QPointer<AbstractButton> item(member);

    auto connections = m_connections.take(member);
    utils::disconnectAll(connections);
    member->m_group = nullptr;
    update();
    if (! guard || ! item) return;
    Q_EMIT item->groupChanged();
    if (! guard || ! item) return;

    Q_EMIT buttonsChanged();
}
void ButtonGroup::setCheckState(Qt::CheckState state) {
    if (state == Qt::PartiallyChecked || (isExclusive() && state != Qt::Unchecked)) return;
    QPointer<ButtonGroup> guard(this);
    const auto            items = m_selection.snapshot();
    for (const auto& item : items) {
        if (item && item->group() == this) item->setChecked(state == Qt::Checked);
        if (! guard) return;
    }
}
ButtonGroupAttached::ButtonGroupAttached(QObject* parent)
    : QObject(parent), m_member(qobject_cast<AbstractButton*>(parent)) {
    if (m_member)
        connect(m_member, &AbstractButton::groupChanged, this, &ButtonGroupAttached::groupChanged);
}
ButtonGroup* ButtonGroupAttached::group() const { return m_member ? m_member->group() : nullptr; }
void         ButtonGroupAttached::setGroup(ButtonGroup* value) {
    if (m_member) m_member->setGroup(value);
}
} // namespace qml_material
