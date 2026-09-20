#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QMap>
#include <QSet>
#include <optional>

namespace qml_material
{
// Tracks navigation lifetimes; the view owns presentation and the caller owns page objects.
class PageStackEntries : public QObject {
    Q_OBJECT
public:
    enum class Status
    {
        Inactive,
        Entering,
        Active,
        Exiting
    };
    enum class Operation
    {
        Push,
        Replace,
        Pop,
        Clear
    };
    struct Entry {
        quint64              id = 0;
        QPointer<QQuickItem> item;
        Status               status   = Status::Inactive;
        bool                 retiring = false;
    };
    struct Transition {
        quint64   id        = 0;
        quint64   from      = 0;
        quint64   to        = 0;
        Operation operation = Operation::Push;
    };
    explicit PageStackEntries(QObject* parent = nullptr): QObject(parent) {}
    ~PageStackEntries() override;
    int                       depth() const { return m_stack.size(); }
    quint64                   currentId() const { return m_stack.isEmpty() ? 0 : m_stack.last(); }
    QQuickItem*               currentItem() const { return entry(currentId()).item; }
    Entry                     entry(quint64 id) const { return m_entries.value(id); }
    quint64                   idAt(int index) const { return m_stack.value(index); }
    int                       indexOf(quint64 id) const { return m_stack.indexOf(id); }
    QList<Entry>              entries() const { return m_entries.values(); }
    std::optional<Transition> transition() const { return m_transition; }
    quint64                   push(QQuickItem* item) { return navigate(Operation::Push, item); }
    quint64       replace(QQuickItem* item) { return navigate(Operation::Replace, item); }
    quint64       pop() { return navigate(Operation::Pop, nullptr); }
    quint64       clear() { return navigate(Operation::Clear, nullptr); }
    bool          finish(quint64 transitionId);
    void          shutdown();
    Q_SIGNAL void changed();
    Q_SIGNAL void transitionRequested(quint64 id);
    Q_SIGNAL void entryRemoved(quint64 id, QQuickItem* item);

private:
    quint64                                navigate(Operation, QQuickItem*);
    void                                   retire(quint64);
    Entry                                  erase(quint64);
    QList<Entry>                           settle();
    void                                   destroyed(quint64);
    void                                   notify(const QList<Entry>& removed = {});
    QMap<quint64, Entry>                   m_entries;
    QMap<quint64, QMetaObject::Connection> m_connections;
    QList<quint64>                         m_stack;
    std::optional<Transition>              m_transition;
    QSet<quint64>                          m_destroyed;
    quint64                                m_next_entry         = 0;
    quint64                                m_next_transition    = 0;
    int                                    m_notification_depth = 0;
    bool                                   m_closed             = false;
};
} // namespace qml_material
