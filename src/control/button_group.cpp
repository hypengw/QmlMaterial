#include "qml_material/control/button_group.hpp"
#include <QQmlInfo>

namespace qml_material
{
ButtonGroup::~ButtonGroup() {
    m_destroying = true;
    m_checked    = QSet<const ButtonGroupAttached*> {};
    clear();
}
ButtonGroupAttached* ButtonGroup::qmlAttachedProperties(QObject* object) {
    auto* button = qobject_cast<ToggleButton*>(object);
    if (! button) {
        qmlWarning(object) << "ButtonGroup requires a toggle button";
        return nullptr;
    }
    return new ButtonGroupAttached(button);
}
QQmlListProperty<ToggleButton> ButtonGroup::buttons() {
    return { this,
             this,
             [](QQmlListProperty<ToggleButton>* p, ToggleButton* button) {
                 static_cast<ButtonGroup*>(p->data)->addButton(button);
             },
             [](QQmlListProperty<ToggleButton>* p) -> qsizetype {
                 return static_cast<ButtonGroup*>(p->data)->m_members.size();
             },
             [](QQmlListProperty<ToggleButton>* p, qsizetype index) -> ToggleButton* {
                 const auto member = static_cast<ButtonGroup*>(p->data)->m_members.value(index);
                 return member ? member->button() : nullptr;
             },
             [](QQmlListProperty<ToggleButton>* p) {
                 static_cast<ButtonGroup*>(p->data)->clear();
             } };
}
void ButtonGroup::clear() {
    const QPointer<ButtonGroup> guard(this);
    const auto                  members = m_members;
    for (const auto& member : members) {
        if (member && member->group() == this) member->setGroup(nullptr);
        if (! guard) return;
    }
}
bool ButtonGroup::isChecked(const ButtonGroupAttached* member) const {
    return m_checked.value().contains(member);
}
QList<ToggleButton*> ButtonGroup::checkedButtons() const {
    QList<ToggleButton*> result;
    const auto           checked = m_checked.value();
    for (const auto& member : m_members)
        if (member && member->button() && checked.contains(member)) result.append(member->button());
    return result;
}
ToggleButton* ButtonGroup::checkedButton() const {
    const auto checked = checkedButtons();
    return m_exclusive && ! checked.isEmpty() ? checked.first() : nullptr;
}
Qt::CheckState ButtonGroup::checkState() const {
    const auto checked = m_checked.value();
    return checked.isEmpty()                    ? Qt::Unchecked
           : checked.size() == m_members.size() ? Qt::Checked
                                                : Qt::PartiallyChecked;
}
void ButtonGroup::commit(QSet<const ButtonGroupAttached*> next) {
    if (m_destroying || next == m_checked.value()) return;
    const QPointer<ButtonGroup> guard(this);
    ++m_revision;
    m_checked = std::move(next);
    if (guard) notifySelection();
}
void ButtonGroup::notifySelection() {
    if (m_destroying) return;
    m_dirty = true;
    if (m_notifying) return;
    m_notifying = true;
    const QPointer<ButtonGroup> guard(this);
    while (m_dirty) {
        m_dirty             = false;
        const auto revision = m_revision;
        Q_EMIT checkedButtonsChanged();
        if (! guard) return;
        if (revision != m_revision) continue;
        Q_EMIT checkStateChanged();
        if (! guard) return;
    }
    m_notifying = false;
}
void ButtonGroup::setCheckedButton(ToggleButton* button) {
    setCheckedButtons(button ? QList<ToggleButton*> { button } : QList<ToggleButton*> {});
}
void ButtonGroup::setCheckedButtons(const QList<ToggleButton*>& buttons) {
    if (m_destroying) return;
    if (! m_complete) {
        m_initialSelection = true;
        m_initialButtons.clear();
        for (auto* button : buttons) m_initialButtons.append(button);
        return;
    }
    QSet<const ButtonGroupAttached*> next;
    for (auto* button : buttons) {
        auto* member = button ? qobject_cast<ButtonGroupAttached*>(
                                    qmlAttachedPropertiesObject<ButtonGroup>(button, false))
                              : nullptr;
        if (! member || member->group() != this) return;
        next.insert(member);
    }
    if (m_exclusive && next.size() > 1) return;
    commit(std::move(next));
}
void ButtonGroup::componentComplete() {
    m_complete = true;
    if (! m_initialSelection) return;
    QList<ToggleButton*> initial;
    for (const auto& button : m_initialButtons)
        if (button) initial.append(button);
    m_initialSelection = false;
    m_initialButtons.clear();
    setCheckedButtons(initial);
}
void ButtonGroup::setExclusive(bool value) {
    if (m_exclusive == value || m_destroying) return;
    m_exclusive = value;
    const QPointer<ButtonGroup> guard(this);
    const auto                  revision = m_revision;
    if (value && m_checked.value().size() > 1) {
        const auto buttons = checkedButtons();
        setCheckedButton(buttons.isEmpty() ? nullptr : buttons.first());
    }
    if (guard && m_revision == revision) notifySelection();
    if (guard) Q_EMIT exclusiveChanged();
}
void ButtonGroup::setCheckState(Qt::CheckState value) {
    if (value != Qt::Unchecked && (value != Qt::Checked || m_exclusive)) return;
    QSet<const ButtonGroupAttached*> next;
    if (value == Qt::Checked)
        for (const auto& member : m_members)
            if (member) next.insert(member);
    commit(std::move(next));
}
void ButtonGroup::requestChecked(ToggleButton* button, bool value) {
    if (m_destroying || ! button || ! button->isEnabled() || ! button->isVisible()) return;
    auto* member =
        qobject_cast<ButtonGroupAttached*>(qmlAttachedPropertiesObject<ButtonGroup>(button, false));
    if (! member || member->group() != this || (! value && m_exclusive && isChecked(member)))
        return;
    auto next = m_checked.value();
    if (value) {
        if (m_exclusive) next.clear();
        next.insert(member);
    } else {
        next.remove(member);
    }
    commit(std::move(next));
}
void ButtonGroup::addButton(ToggleButton* button) {
    if (! button || m_destroying) return;
    auto* member =
        qobject_cast<ButtonGroupAttached*>(qmlAttachedPropertiesObject<ButtonGroup>(button, true));
    member->setGroup(this);
}
void ButtonGroup::removeButton(ToggleButton* button) {
    if (! button) return;
    auto* member =
        qobject_cast<ButtonGroupAttached*>(qmlAttachedPropertiesObject<ButtonGroup>(button, false));
    if (member && member->group() == this) member->setGroup(nullptr);
}
ButtonGroupAttached::ButtonGroupAttached(ToggleButton* button): QObject(button), m_button(button) {
    m_checkedNotifier = m_checked.addNotifier([this] {
        Q_EMIT checkedChanged();
    });
    m_checked.setBinding([this] {
        const auto owner = m_group.value();
        return owner && owner->isChecked(this);
    });
    connect(button, &QObject::destroyed, this, [this] {
        m_button = nullptr;
        setGroup(nullptr);
    });
}
ButtonGroupAttached::~ButtonGroupAttached() {
    m_destroying = true;
    m_checked.takeBinding();
    setGroup(nullptr);
}
void ButtonGroupAttached::setGroup(ButtonGroup* value) {
    if (group() == value || (value && value->m_destroying)) return;
    const QPointer<ButtonGroupAttached> guard(this);
    const QPointer<ButtonGroup>         previous(group()), next(value);
    const auto                          revision = ++m_revision;
    disconnect(m_requestConnection);
    {
        const QScopedPropertyUpdateGroup update;
        if (previous) {
            previous->m_members.removeOne(this);
            auto checked = previous->m_checked.value();
            checked.remove(this);
            ++previous->m_revision;
            previous->m_checked = std::move(checked);
        }
        if (next) {
            next->m_members.append(this);
            ++next->m_revision;
        }
        m_group = QPointer<ButtonGroup>(value);
        if (next && m_button) {
            m_requestConnection = connect(m_button,
                                          &ToggleButton::checkedChangeRequested,
                                          this,
                                          [owner = next, button = m_button](bool value) {
                                              if (owner && button)
                                                  owner->requestChecked(button, value);
                                          });
        }
    }
    if (guard && m_revision == revision && m_button) m_button->cancelActivation();
    if (previous) {
        previous->notifySelection();
        if (previous && ! previous->m_destroying) Q_EMIT previous->buttonsChanged();
    }
    if (next) {
        next->notifySelection();
        if (next && ! next->m_destroying) Q_EMIT next->buttonsChanged();
    }
    if (guard && m_revision == revision && ! m_destroying) Q_EMIT groupChanged();
}
} // namespace qml_material
