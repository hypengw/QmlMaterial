#pragma once

#include <QList>
#include <QPointer>

namespace qml_material::utils
{
// Non-owning selection state shared by action and button groups.
template<class T>
class SelectionSet {
public:
    QList<T*> members;
    bool      exclusive = true;
    T*        selected  = nullptr;
    quint64   revision  = 0;

    QList<QPointer<T>> snapshot() const {
        QList<QPointer<T>> result;
        for (auto* member : members) result.append(member);
        return result;
    }

    void normalize(QObject* owner, T* preferred = nullptr) {
        const auto serial = ++revision;
        if (! exclusive) {
            selected = nullptr;
            return;
        }
        if (! members.contains(selected) || ! selected->isChecked()) selected = nullptr;
        if (preferred && members.contains(preferred) && preferred->isChecked())
            selected = preferred;
        if (! selected) {
            for (auto* member : members)
                if (member->isChecked()) {
                    selected = member;
                    break;
                }
        }
        QPointer<QObject> guard(owner);
        const auto        items = snapshot();
        for (const auto& member : items) {
            if (member && members.contains(member) && member != selected && member->isChecked())
                member->setChecked(false);
            if (! guard || revision != serial) return;
        }
    }

    Qt::CheckState checkState() const {
        qsizetype checked = 0;
        for (auto* member : members) checked += member->isChecked();
        return ! checked                   ? Qt::Unchecked
               : checked == members.size() ? Qt::Checked
                                           : Qt::PartiallyChecked;
    }
};
} // namespace qml_material::utils
