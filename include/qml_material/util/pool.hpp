#pragma once

#include <set>
#include <map>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlIncubator>
#include <QtQuick/QQuickItem>

namespace qml_material
{

class Pool;
class PoolIncubator : public QQmlIncubator {
public:
    PoolIncubator(Pool* p, qint64 id, IncubationMode mode);

protected:
    void statusChanged(Status) override;
    void setInitialState(QObject*) override;

private:
    Pool*  m_pool;
    qint64 m_id;
};

class Pool : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged FINAL)

    friend class PoolIncubator;

public:
    Pool(QObject* parent = nullptr);
    ~Pool();

    Q_INVOKABLE void add(QQmlComponent* comp, const QVariantMap&, bool cache);
    Q_INVOKABLE void add(const QUrl& url, const QVariantMap&, bool cache);
    Q_INVOKABLE void addFromModule(const QString& uri, const QString& typeName, const QVariantMap&,
                                   bool cache);

    Q_INVOKABLE void addWithKey(const QString& key, QQmlComponent* comp, const QVariantMap&,
                                bool cache);
    Q_INVOKABLE void addWithKey(const QString& key, const QUrl& url, const QVariantMap&,
                                bool cache);
    Q_INVOKABLE void addFromModuleWithKey(const QString& key, const QString& uri,
                                          const QString& typeName, const QVariantMap&, bool cache);

    Q_SIGNAL void objectAdded(QObject* obj, bool is_cache);

    auto          async() const -> bool;
    void          setAsync(bool);
    Q_SIGNAL void asyncChanged(bool);

    Q_INVOKABLE bool removeObject(QObject*);

private:
    Q_SIGNAL void queueAdded(qint64);
    Q_SLOT void   onQueueAdded(qint64);
    Q_SLOT void   onUncacheObjectDeleted(QObject*);

    struct Task {
        QString  key;
        bool     cache;
        QObject* object;

        QPointer<QQmlComponent> component {};
        PoolIncubator*          incubator { nullptr };
    };

    void incubatorStateChanged(qint64 id, QQmlIncubator::Status status);
    void setInitialState(QObject* o);
    auto genSerial() const -> qint64;
    void onComponentProgress(qint64 id, qreal);
    void onComponentLoaded(qint64 id);
    auto createComponent() -> QQmlComponent*;
    auto tryCache(QStringView, bool) -> bool;

    using task_iterator = std::map<qint64, Task>::iterator;
    void clearTask(task_iterator);

    mutable qint64                                    m_serial;
    std::map<qint64, Task>                            m_tasks;
    std::map<QString, QPointer<QObject>, std::less<>> m_objs;
    std::set<QObject*>                                m_uncache_objs;

    bool m_async;
};

} // namespace qml_material