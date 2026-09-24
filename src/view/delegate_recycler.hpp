#pragma once

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QPointer>
#include "qml_material/model/lazy_row.hpp"

namespace qml_material
{
class DelegateRecycler : public QObject {
public:
    explicit DelegateRecycler(QQmlEngine*, QObject* parent = nullptr);
    ~DelegateRecycler() override;
    QQuickItem* acquire(QQmlComponent*, const ItemSnapshotPtr&, int index);
    bool        rebind(QQuickItem*, const ItemSnapshotPtr&, int index);
    void        attach(QQuickItem*, QQuickItem* parent);
    void        layout(QQuickItem*, const QRectF&);
    void        release(QQuickItem*);
    void        clear();
    void        setPoolLimit(int);
    int         poolSize() const;
    LazyRow*    row(QQuickItem*) const;
    QString     errorString() const { return m_error; }

private:
    struct Entry {
        ~Entry();
        QPointer<QQuickItem>    item;
        QPointer<QQuickItem>    host;
        QPointer<LazyRow>       row;
        QPointer<QQmlComponent> component;
        QString                 type;
        bool                    idle     = false;
        bool                    updating = false;
    };
    std::shared_ptr<Entry>        entry(QQuickItem*) const;
    void                          trim();
    QPointer<QQmlEngine>          m_engine;
    QList<std::shared_ptr<Entry>> m_entries;
    QString                       m_error;
    quint64                       m_generation = 0;
    int                           m_poolLimit  = 16;
    bool                          m_clearing   = false;
};
} // namespace qml_material
