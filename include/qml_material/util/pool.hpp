#pragma once

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>

#include <QtCore/QPointer>
#include <QtCore/QVariant>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlIncubator>

#include "qml_material/export.hpp"

namespace qml_material
{

auto pool_object_count() -> std::size_t;

class Pool;

class QML_MATERIAL_API PoolRequest : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(PoolRequest)
    QML_UNCREATABLE("PoolRequest objects are returned by Pool.request()")

    Q_PROPERTY(qint64 id READ id CONSTANT FINAL)
    Q_PROPERTY(QVariant key READ key CONSTANT FINAL)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged FINAL)
    Q_PROPERTY(QObject* object READ object NOTIFY objectChanged FINAL)
    Q_PROPERTY(bool cached READ cached NOTIFY cachedChanged FINAL)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged FINAL)

public:
    enum Status
    {
        Null,
        Loading,
        Ready,
        Error,
        Cancelled,
        Released,
    };
    Q_ENUM(Status)

    auto id() const -> qint64;
    auto key() const -> QVariant;
    auto status() const -> Status;
    auto progress() const -> qreal;
    auto object() const -> QObject*;
    auto cached() const -> bool;
    auto errorString() const -> QString;

    Q_INVOKABLE void cancel();
    Q_INVOKABLE void forceCompletion();
    Q_INVOKABLE void release();

    Q_SIGNAL void statusChanged();
    Q_SIGNAL void progressChanged();
    Q_SIGNAL void objectChanged();
    Q_SIGNAL void cachedChanged();
    Q_SIGNAL void errorStringChanged();

private:
    friend class Pool;

    PoolRequest(Pool* pool, qint64 id, QVariant key);

    void setStatus(Status value);
    void setProgress(qreal value);
    void setObject(QObject* value);
    void setCached(bool value);
    void setErrorString(QString value);

    QPointer<Pool>    m_pool;
    qint64            m_id;
    QVariant          m_key;
    Status            m_status { Null };
    qreal             m_progress { 0 };
    QPointer<QObject> m_object;
    bool              m_cached { false };
    QString           m_errorString;
};

class PoolIncubator : public QQmlIncubator {
public:
    PoolIncubator(Pool* pool, qint64 id, IncubationMode mode);

protected:
    void statusChanged(Status status) override;
    void setInitialState(QObject* object) override;

private:
    Pool*  m_pool;
    qint64 m_id;
};

class QML_MATERIAL_API Pool : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool async READ async WRITE setAsync NOTIFY asyncChanged FINAL)

    friend class PoolIncubator;
    friend class PoolRequest;

public:
    enum IncubationMode
    {
        Asynchronous,
        AsynchronousIfNested,
        Synchronous,
    };
    Q_ENUM(IncubationMode)

    explicit Pool(QObject* parent = nullptr);
    ~Pool() override;

    Q_INVOKABLE PoolRequest* request(const QVariant&    source,
                                     const QVariantMap& initialProperties = {},
                                     const QVariant&    key               = {},
                                     IncubationMode     mode              = AsynchronousIfNested);

    Q_INVOKABLE bool     contains(const QString& key) const;
    Q_INVOKABLE QObject* get(const QString& key) const;
    Q_INVOKABLE bool     evict(const QVariant& key);
    Q_INVOKABLE void     clear();

    Q_INVOKABLE void add(const QVariant& source, const QVariantMap& initialProperties,
                         bool autoKey = false);
    Q_INVOKABLE void addWithKey(const QString& key, const QVariant& source,
                                const QVariantMap& initialProperties);
    Q_SIGNAL void    objectAdded(QObject* object, const QVariant& key);

    auto          async() const -> bool;
    void          setAsync(bool value);
    Q_SIGNAL void asyncChanged(bool value);

    Q_INVOKABLE bool removeObject(QObject* object);

    void add(std::optional<QStringView> key, QQmlComponent* component,
             const QVariantMap& initialProperties);

private:
    struct Task {
        qint64                         id;
        QVariant                       source;
        QVariantMap                    initialProperties;
        QString                        key;
        bool                           hasKey { false };
        IncubationMode                 mode;
        QPointer<PoolRequest>          request;
        QPointer<QQmlComponent>        component;
        std::unique_ptr<QQmlComponent> ownedComponent;
        std::unique_ptr<PoolIncubator> incubator;
        QString                        sourceError;
        bool                           terminal { false };
    };

    struct CacheEntry {
        QPointer<QObject>     object;
        QPointer<PoolRequest> lease;
        qint64                generation;
        bool                  evictOnRelease { false };
    };

    void incubatorStateChanged(qint64 id, QQmlIncubator::Status status);
    void setInitialState(qint64 id, QObject* object);
    void onComponentStatusChanged(qint64 id);
    void onComponentProgressChanged(qint64 id, qreal progress);
    void startIncubation(qint64 id);
    void forceRequest(PoolRequest* request);
    void cancelRequest(PoolRequest* request);
    void releaseRequest(PoolRequest* request);

    auto createComponent(Task& task, QQmlComponent::CompilationMode mode) -> bool;
    auto taskError(const Task& task) const -> QString;
    auto genSerial() -> qint64;
    auto toQtMode(IncubationMode mode) const -> QQmlIncubator::IncubationMode;

    void finishReady(qint64 id, QObject* object);
    void finishError(qint64 id, QString error);
    void finishCancelled(qint64 id);
    void scheduleTaskCleanup(qint64 id);
    void removeInflightKey(const Task& task);

    void onCacheObjectDestroyed(const QString& key, qint64 generation);
    void onUncachedObjectDestroyed(QObject* object);
    void attachLegacy(PoolRequest* request);
    void queueLegacyDrain();
    void drainLegacy();

    std::map<qint64, std::unique_ptr<Task>>               m_tasks;
    std::map<QString, CacheEntry, std::less<>>            m_cache;
    std::map<QString, QPointer<PoolRequest>, std::less<>> m_inflightKeys;
    std::map<QObject*, QPointer<PoolRequest>>             m_uncached;
    std::deque<QPointer<PoolRequest>>                     m_legacyQueue;

    qint64 m_serial { 0 };
    bool   m_async { true };
    bool   m_destroying { false };
    bool   m_legacyDrainPending { false };
};

} // namespace qml_material
