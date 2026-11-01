#pragma once

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QPointer>
#include <QQmlIncubator>
#include <QSet>
#include <functional>
#include "qml_material/model/lazy_row.hpp"

namespace qml_material
{
class DelegateRecycler : public QObject {
public:
    struct Placement {
        QPointer<QQuickItem> parent, view;
        QRectF               geometry;
        Qt::Orientation      orientation = Qt::Vertical;
        LazyRow::Usage       usage       = LazyRow::Cached;
        bool                 fixedExtent = false;
        bool                 valid       = false;
    };
    explicit DelegateRecycler(QQmlEngine*, QObject* parent = nullptr);
    ~DelegateRecycler() override;
    QQuickItem* acquire(QQmlComponent*, const ItemSnapshotPtr&, int index);
    QQuickItem* acquire(QQmlComponent*, const ItemSnapshotPtr&, int index, const Placement&,
                        bool asynchronous);
    bool        rebind(QQuickItem*, const ItemSnapshotPtr&, int index);
    bool        rebind(QQuickItem*, const ItemSnapshotPtr&, int index, const Placement&);
    void        retainPreparations(QQmlComponent*, const ItemSnapshotPtr&, const QSet<QString>&);
    void        cancelPreparations();
    bool        pending(const QString&) const;
    int         pendingCount() const;
    void        setReadyCallback(std::function<void()> callback) { m_ready = std::move(callback); }
    void        attach(QQuickItem*, QQuickItem* parent);
    void        layout(QQuickItem*, const QRectF&, Qt::Orientation = Qt::Vertical,
                       bool fixedExtent = false);
    void        release(QQuickItem*);
    void        clear();
    void        setPoolLimit(int);
    void        beginUpdate();
    void        endUpdate();
    int         poolSize() const;
    LazyRow*    row(QQuickItem*) const;
    QString     errorString() const { return m_error; }

private:
    class Incubator;
    struct Entry {
        ~Entry();
        QPointer<QQuickItem>       item;
        QPointer<QObject>          object;
        QPointer<QQuickItem>       host;
        QPointer<LazyRow>          row;
        QPointer<QQmlComponent>    component;
        QString                    type;
        QString                    key, error;
        QQuickItem*                itemIdentity = nullptr;
        Placement                  placement;
        std::shared_ptr<Incubator> incubator;
        bool                       preparing = false;
        bool                       cancelled = false;
        bool                       idle      = false;
        bool                       updating  = false;
    };
    std::shared_ptr<Entry>                   entry(QQuickItem*) const;
    bool                                     place(const std::shared_ptr<Entry>&, const Placement&);
    void                                     remove(const std::shared_ptr<Entry>&);
    void                                     finishPreparation(const std::shared_ptr<Entry>&);
    static void                              retire(const std::shared_ptr<Entry>&, QQmlEngine*);
    void                                     notifyReady();
    void                                     trim();
    QPointer<QQmlEngine>                     m_engine;
    QList<std::shared_ptr<Entry>>            m_entries;
    QHash<QQuickItem*, std::weak_ptr<Entry>> m_items;
    QHash<QString, std::shared_ptr<Entry>>   m_pending;
    std::function<void()>                    m_ready;
    QString                                  m_error;
    quint64                                  m_generation  = 0;
    int                                      m_poolLimit   = 16;
    int                                      m_updateDepth = 0;
    bool                                     m_clearing    = false;
    bool                                     m_readyQueued = false;
};
} // namespace qml_material
