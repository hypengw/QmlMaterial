#include "page_stack_entries_p.hpp"

namespace qml_material
{
PageStackEntries::~PageStackEntries() {
    for (const auto& connection : m_connections) disconnect(connection);
}

quint64 PageStackEntries::navigate(Operation operation, QQuickItem* item) {
    if (m_closed || m_notification_depth || m_transition) return 0;
    if (operation == Operation::Push || operation == Operation::Replace) {
        if (! item) return 0;
        for (const auto& entry : m_entries)
            if (entry.item == item) return 0;
    } else if ((operation == Operation::Pop && depth() < 2) || m_stack.isEmpty()) {
        return 0;
    }
    const auto from = currentId();
    if (from) m_entries[from].status = Status::Exiting;
    if (operation == Operation::Replace || operation == Operation::Pop) {
        if (from) retire(from);
    } else if (operation == Operation::Clear) {
        const auto stack = m_stack;
        for (auto id : stack) retire(id);
    }
    if (item) {
        const auto id = ++m_next_entry;
        m_entries.insert(id, { id, item, Status::Entering, false });
        m_stack.append(id);
        m_connections.insert(id, connect(item, &QObject::destroyed, this, [this, id] {
                                 destroyed(id);
                             }));
    } else if (currentId()) {
        m_entries[currentId()].status = Status::Entering;
    }
    const auto token = ++m_next_transition;
    m_transition     = Transition { token, from, currentId(), operation };
    QPointer<PageStackEntries> guard(this);
    notify();
    if (guard && m_transition && m_transition->id == token) Q_EMIT transitionRequested(token);
    return token;
}

void PageStackEntries::retire(quint64 id) {
    m_stack.removeOne(id);
    m_entries[id].retiring = true;
}
PageStackEntries::Entry PageStackEntries::erase(quint64 id) {
    disconnect(m_connections.take(id));
    m_stack.removeOne(id);
    m_destroyed.remove(id);
    return m_entries.take(id);
}
QList<PageStackEntries::Entry> PageStackEntries::settle() {
    m_transition.reset();
    QList<Entry> removed;
    const auto   ids = m_entries.keys();
    for (auto id : ids) {
        if (m_entries[id].retiring)
            removed.append(erase(id));
        else
            m_entries[id].status = id == currentId() ? Status::Active : Status::Inactive;
    }
    return removed;
}
bool PageStackEntries::finish(quint64 id) {
    if (m_notification_depth || ! m_transition || m_transition->id != id) return false;
    const auto removed = settle();
    notify(removed);
    return true;
}
void PageStackEntries::shutdown() {
    if (m_closed) return;
    m_closed = true;
    m_transition.reset();
    QList<Entry> removed;
    const auto   ids = m_entries.keys();
    for (auto id : ids) removed.append(erase(id));
    notify(removed);
}
void PageStackEntries::destroyed(quint64 id) {
    if (! m_entries.contains(id)) return;
    if (m_notification_depth) {
        m_destroyed.insert(id);
        return;
    }
    auto removed = QList<Entry> { erase(id) };
    removed.append(settle());
    notify(removed);
}
void PageStackEntries::notify(const QList<Entry>& removed) {
    ++m_notification_depth;
    QPointer<PageStackEntries> guard(this);
    Q_EMIT changed();
    if (! guard) return;
    for (const auto& entry : removed) {
        Q_EMIT entryRemoved(entry.id, entry.item);
        if (! guard) return;
    }
    --m_notification_depth;
    if (m_notification_depth) return;
    while (! m_destroyed.isEmpty()) {
        const auto id = *m_destroyed.begin();
        m_destroyed.remove(id);
        destroyed(id);
        if (! guard) return;
    }
}
} // namespace qml_material
