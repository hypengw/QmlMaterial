#include <QCoreApplication>
#include <QFile>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlIncubator>
#include <QQuickItem>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "qml_material/util/pool.hpp"

namespace
{

void processDeferredDeletes() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

} // namespace

class ObjectPoolTest : public QObject {
    Q_OBJECT

private:
    auto createPoolHost(const QUrl& baseUrl = QUrl(QStringLiteral("qrc:/tests/PoolHost.qml")))
        -> std::unique_ptr<QObject> {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQml
            import Qcm.Material as MD

            QtObject {
                property MD.Pool pool: MD.Pool {}
            }
        )",
                          baseUrl);
        if (component.isError()) qWarning() << component.errors();
        return std::unique_ptr<QObject>(component.create());
    }

    static auto poolFrom(QObject* host) -> qml_material::Pool* {
        return qvariant_cast<qml_material::Pool*>(host->property("pool"));
    }

    auto readyComponent() -> std::unique_ptr<QQmlComponent> {
        auto component = std::make_unique<QQmlComponent>(&m_engine);
        component->setData(R"(
            import QtQml

            QtObject {
                required property int value
                property bool completed: false
                Component.onCompleted: completed = true
            }
        )",
                           QUrl(QStringLiteral("qrc:/tests/ReadyObject.qml")));
        return component;
    }

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_engine.setIncubationController(&m_controller);
    }

    void synchronousRequestAndRelease() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);

        auto component = readyComponent();
        QVERIFY2(! component->isError(), qPrintable(component->errorString()));
        auto* request = pool->request(QVariant::fromValue(component.get()),
                                      { { "value", 42 } },
                                      {},
                                      qml_material::Pool::Synchronous);
        QCOMPARE(request->status(), qml_material::PoolRequest::Ready);
        QCOMPARE(request->progress(), 1.0);
        QVERIFY(request->object());
        QCOMPARE(request->object()->property("value").toInt(), 42);
        QVERIFY(request->object()->property("completed").toBool());

        QPointer<QObject> object = request->object();
        request->release();
        QCOMPARE(request->status(), qml_material::PoolRequest::Released);
        processDeferredDeletes();
        QVERIFY(object.isNull());
    }

    void relativeUrlAndModuleType() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile file(directory.filePath(QStringLiteral("Relative.qml")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(R"(
            import QtQml
            QtObject { property string marker: "relative" }
        )") > 0);
        file.close();

        auto host =
            createPoolHost(QUrl::fromLocalFile(directory.filePath(QStringLiteral("PoolHost.qml"))));
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);

        auto* relative =
            pool->request(QStringLiteral("Relative.qml"), {}, {}, qml_material::Pool::Synchronous);
        QCOMPARE(relative->status(), qml_material::PoolRequest::Ready);
        QCOMPARE(relative->object()->property("marker").toString(), QStringLiteral("relative"));
        relative->release();

        auto* module =
            pool->request(QStringLiteral("QtQuick/Item"), {}, {}, qml_material::Pool::Synchronous);
        QCOMPARE(module->status(), qml_material::PoolRequest::Ready);
        QVERIFY(qobject_cast<QQuickItem*>(module->object()));
        module->release();
        processDeferredDeletes();
    }

    void asynchronousCancelAndForceCompletion() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);
        auto component = readyComponent();

        QPointer<qml_material::PoolRequest> cancelled =
            pool->request(QVariant::fromValue(component.get()),
                          { { "value", 1 } },
                          {},
                          qml_material::Pool::Asynchronous);
        QCOMPARE(cancelled->status(), qml_material::PoolRequest::Loading);
        cancelled->cancel();
        QCOMPARE(cancelled->status(), qml_material::PoolRequest::Cancelled);
        processDeferredDeletes();
        QVERIFY(cancelled.isNull());

        auto* forced = pool->request(QVariant::fromValue(component.get()),
                                     { { "value", 2 } },
                                     {},
                                     qml_material::Pool::Asynchronous);
        QCOMPARE(forced->status(), qml_material::PoolRequest::Loading);
        forced->forceCompletion();
        QCOMPARE(forced->status(), qml_material::PoolRequest::Ready);
        QCOMPARE(forced->object()->property("value").toInt(), 2);
        forced->release();
        processDeferredDeletes();

        QPointer<qml_material::PoolRequest> abandoned =
            pool->request(QVariant::fromValue(component.get()),
                          { { "value", 3 } },
                          {},
                          qml_material::Pool::Asynchronous);
        QCOMPARE(abandoned->status(), qml_material::PoolRequest::Loading);
        abandoned->deleteLater();
        processDeferredDeletes();
        QVERIFY(abandoned.isNull());
        QCOMPARE(m_controller.incubatingObjectCount(), 0);
    }

    void asynchronousSourceForceCompletion() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile file(directory.filePath(QStringLiteral("Async.qml")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(R"(
            import QtQml
            QtObject { property string marker: "async-source" }
        )") > 0);
        file.close();

        auto host =
            createPoolHost(QUrl::fromLocalFile(directory.filePath(QStringLiteral("PoolHost.qml"))));
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);

        auto* request =
            pool->request(QStringLiteral("Async.qml"), {}, {}, qml_material::Pool::Asynchronous);
        QCOMPARE(request->status(), qml_material::PoolRequest::Loading);
        request->forceCompletion();
        QCOMPARE(request->status(), qml_material::PoolRequest::Ready);
        QCOMPARE(request->object()->property("marker").toString(), QStringLiteral("async-source"));
        request->release();
        processDeferredDeletes();
    }

    void errorsDoNotBlockLaterRequests() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);

        QQmlComponent invalid(&m_engine);
        invalid.setData("import QtQml; QtObject { broken", QUrl(QStringLiteral("invalid.qml")));
        QVERIFY(invalid.isError());
        auto* failed =
            pool->request(QVariant::fromValue(&invalid), {}, {}, qml_material::Pool::Synchronous);
        QCOMPARE(failed->status(), qml_material::PoolRequest::Error);
        QVERIFY(! failed->errorString().isEmpty());
        failed->release();

        QQmlComponent missingRequired(&m_engine);
        missingRequired.setData(R"(
            import QtQml
            QtObject { required property int value }
        )",
                                QUrl(QStringLiteral("missing-required.qml")));
        QVERIFY(! missingRequired.isError());
        auto* incubationError = pool->request(
            QVariant::fromValue(&missingRequired), {}, {}, qml_material::Pool::Synchronous);
        QCOMPARE(incubationError->status(), qml_material::PoolRequest::Error);
        QVERIFY(! incubationError->errorString().isEmpty());
        incubationError->release();

        auto  component = readyComponent();
        auto* ready     = pool->request(QVariant::fromValue(component.get()),
                                        { { "value", 3 } },
                                        {},
                                        qml_material::Pool::Synchronous);
        QCOMPARE(ready->status(), qml_material::PoolRequest::Ready);
        ready->release();
        processDeferredDeletes();
    }

    void keyedLeaseAndEviction() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);
        auto component = readyComponent();

        auto* first = pool->request(QVariant::fromValue(component.get()),
                                    { { "value", 4 } },
                                    "page",
                                    qml_material::Pool::Synchronous);
        QCOMPARE(first->status(), qml_material::PoolRequest::Ready);
        QPointer<QObject> object = first->object();

        auto* busy = pool->request(QVariant::fromValue(component.get()),
                                   { { "value", 5 } },
                                   "page",
                                   qml_material::Pool::Synchronous);
        QCOMPARE(busy->status(), qml_material::PoolRequest::Error);
        QVERIFY(busy->errorString().contains(QStringLiteral("already in use")));
        busy->release();

        first->release();
        processDeferredDeletes();
        QVERIFY(object);
        QVERIFY(pool->contains(QStringLiteral("page")));

        auto* cached = pool->request(QVariant::fromValue(component.get()),
                                     { { "value", 6 } },
                                     "page",
                                     qml_material::Pool::Synchronous);
        QCOMPARE(cached->status(), qml_material::PoolRequest::Ready);
        QCOMPARE(cached->object(), object.data());
        QCOMPARE(cached->object()->property("value").toInt(), 4);
        cached->release();
        QVERIFY(pool->evict(QStringLiteral("page")));
        processDeferredDeletes();
        QVERIFY(object.isNull());
        QVERIFY(! pool->contains(QStringLiteral("page")));

        auto*             clearWhileLeased = pool->request(QVariant::fromValue(component.get()),
                                                           { { "value", 7 } },
                                                           "clear-page",
                                                           qml_material::Pool::Synchronous);
        QPointer<QObject> clearObject      = clearWhileLeased->object();
        pool->clear();
        QVERIFY(clearObject);
        clearWhileLeased->release();
        processDeferredDeletes();
        QVERIFY(clearObject.isNull());
    }

    void duplicateInflightAndClear() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);
        auto component = readyComponent();

        QPointer<qml_material::PoolRequest> first =
            pool->request(QVariant::fromValue(component.get()),
                          { { "value", 7 } },
                          "pending",
                          qml_material::Pool::Asynchronous);
        auto* duplicate = pool->request(QVariant::fromValue(component.get()),
                                        { { "value", 8 } },
                                        "pending",
                                        qml_material::Pool::Asynchronous);
        QCOMPARE(duplicate, first.data());
        QCOMPARE(first->status(), qml_material::PoolRequest::Loading);

        pool->clear();
        QCOMPARE(first->status(), qml_material::PoolRequest::Cancelled);
        processDeferredDeletes();
        QVERIFY(first.isNull());
        QVERIFY(! pool->contains(QStringLiteral("pending")));
    }

    void poolDestructionCancelsPending() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);
        auto component = readyComponent();

        QPointer<qml_material::PoolRequest> request =
            pool->request(QVariant::fromValue(component.get()),
                          { { "value", 8 } },
                          {},
                          qml_material::Pool::Asynchronous);
        QCOMPARE(request->status(), qml_material::PoolRequest::Loading);
        QVERIFY(m_controller.incubatingObjectCount() > 0);
        host.reset();
        processDeferredDeletes();
        QVERIFY(request.isNull());
        QCOMPARE(m_controller.incubatingObjectCount(), 0);
    }

    void externallyDestroyedLeaseBecomesError() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);
        auto component = readyComponent();

        auto* request = pool->request(QVariant::fromValue(component.get()),
                                      { { "value", 9 } },
                                      "page",
                                      qml_material::Pool::Synchronous);
        auto* object  = request->object();
        QVERIFY(object);
        delete object;
        QCOMPARE(request->status(), qml_material::PoolRequest::Error);
        QVERIFY(! request->errorString().isEmpty());
        QVERIFY(! pool->contains(QStringLiteral("page")));
        request->release();
        processDeferredDeletes();
    }

    void legacyQueueContinuesAfterError() {
        auto host = createPoolHost();
        QVERIFY(host);
        auto* pool = poolFrom(host.get());
        QVERIFY(pool);
        pool->setAsync(false);
        QSignalSpy added(pool, &qml_material::Pool::objectAdded);

        QQmlComponent invalid(&m_engine);
        invalid.setData("import QtQml; QtObject { broken", QUrl(QStringLiteral("invalid.qml")));
        auto component = readyComponent();
        pool->add(QVariant::fromValue(&invalid), {});
        pool->add(QVariant::fromValue(component.get()), { { "value", 10 } });

        QTRY_COMPARE(added.count(), 1);
        auto* object = qvariant_cast<QObject*>(added.at(0).at(0));
        QVERIFY(object);
        QCOMPARE(object->property("value").toInt(), 10);
        QVERIFY(pool->removeObject(object));
        processDeferredDeletes();
    }

private:
    QQmlEngine               m_engine;
    QQmlIncubationController m_controller;
};

int run_object_pool(int argc, char** argv) {
    ObjectPoolTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "object_pool.moc"
