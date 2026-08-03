#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlIncubator>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "qml_material/control/popup_presenter.hpp"
#include "qml_material/util/pool.hpp"

namespace
{

void processDeferredDeletes() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

} // namespace

class DynamicCreationTest : public QObject {
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

    auto createPresentationHost() -> std::unique_ptr<QObject> {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: root
                width: 640
                height: 480
                property alias presenter: presenter
                property bool popupReady: false
                property var heldPresentation: null
                property var controlledPopup: null
                signal submit(var result)
                signal dismiss()
                signal reportFailure(string error)

                MD.PopupPresenter {
                    id: presenter
                    host: root
                    incubationMode: MD.Pool.Synchronous
                }

                property Component normalSource: Component {
                    MD.Popup {
                        enter: null
                        exit: null
                    }
                }
                property Component waitingSource: Component {
                    MD.Popup {
                        MD.Presentation.ready: root.popupReady
                        enter: null
                        exit: null
                    }
                }
                property Component controlledSource: Component {
                    MD.Popup {
                        id: controlledPopup
                        enter: null
                        exit: null
                        Component.onCompleted: root.controlledPopup = controlledPopup
                        Connections {
                            target: root
                            function onDismiss() {
                                controlledPopup.close()
                            }
                        }
                    }
                }
                property Component reportingSource: Component {
                    MD.Popup {
                        id: reportingPopup
                        MD.Presentation.ready: false
                        Connections {
                            target: root
                            function onReportFailure(error) {
                                reportingPopup.MD.Presentation.fail(error)
                            }
                        }
                    }
                }
                property Component openReportingSource: Component {
                    MD.Popup {
                        id: openReportingPopup
                        enter: null
                        exit: null
                        Connections {
                            target: root
                            function onReportFailure(error) {
                                openReportingPopup.MD.Presentation.fail(error)
                            }
                        }
                    }
                }
                property Component failedSource: Component {
                    MD.Popup {
                        Component.onCompleted: MD.Presentation.fail("early failure")
                    }
                }
                property Component resultSource: Component {
                    MD.Popup {
                        id: resultPopup
                        enter: null
                        exit: null
                        function submitResult(value) {
                            MD.Presentation.complete(value)
                        }
                        Connections {
                            target: root
                            function onSubmit(value) {
                                resultPopup.submitResult(value)
                            }
                        }
                    }
                }
                property Component invalidSource: Component {
                    Item {}
                }
                property Component requiredSource: Component {
                    MD.Popup {
                        required property int missingValue
                    }
                }

                function presentNormal() {
                    return presenter.present(normalSource)
                }
                function presentWaiting() {
                    return presenter.present(waitingSource)
                }
                function presentControlled() {
                    return presenter.present(controlledSource)
                }
                function presentReporting() {
                    return presenter.present(reportingSource)
                }
                function presentOpenReporting() {
                    return presenter.present(openReportingSource)
                }
                function presentFailed() {
                    return presenter.present(failedSource)
                }
                function presentResult() {
                    return presenter.present(resultSource)
                }
                function presentInvalid() {
                    return presenter.present(invalidSource)
                }
                function presentRequired() {
                    return presenter.present(requiredSource)
                }
                function presentMissing() {
                    return presenter.present("MissingPopup.qml")
                }
                function presentWithParent() {
                    return presenter.present(normalSource, { parent: root })
                }
                function holdNormal() {
                    heldPresentation = presenter.present(normalSource)
                    return heldPresentation
                }
                function clearHeld() {
                    heldPresentation = null
                    gc()
                }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/PresentationHost.qml")));
        if (component.isError()) qWarning() << component.errors();
        auto result = std::unique_ptr<QObject>(component.create());
        if (auto* item = qobject_cast<QQuickItem*>(result.get()))
            item->setParentItem(m_window.contentItem());
        return result;
    }

    auto createRelativePresentationHost(const QUrl& baseUrl) -> std::unique_ptr<QObject> {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                id: root
                width: 640
                height: 480
                property alias presenter: presenter

                MD.PopupPresenter {
                    id: presenter
                    host: root
                    incubationMode: MD.Pool.Synchronous
                }

                function presentRelative() {
                    return presenter.present("RelativePopup.qml")
                }
            }
        )",
                          baseUrl);
        if (component.isError()) qWarning() << component.errors();
        auto result = std::unique_ptr<QObject>(component.create());
        if (auto* item = qobject_cast<QQuickItem*>(result.get()))
            item->setParentItem(m_window.contentItem());
        return result;
    }

    static auto invokePresentation(QObject* host, const char* method)
        -> qml_material::PopupPresentation* {
        QVariant result;
        if (! QMetaObject::invokeMethod(host, method, Q_RETURN_ARG(QVariant, result)))
            return nullptr;
        return qvariant_cast<qml_material::PopupPresentation*>(result);
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

private Q_SLOTS:
    void initTestCase() {
        m_engine.addImportPath(QCoreApplication::applicationDirPath() +
                               QStringLiteral("/../qml_modules"));
        const auto importPath = qEnvironmentVariable("QML_IMPORT_PATH");
        for (const auto& path : importPath.split(QDir::listSeparator(), Qt::SkipEmptyParts)) {
            m_engine.addImportPath(path);
        }
        m_engine.setIncubationController(&m_controller);
        m_window.setGeometry(0, 0, 640, 480);
        m_window.create();
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

    void popupDeferredOpenAndRejectCleanup() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 640
                height: 480
                property alias popup: popup
                property alias defaultPopup: defaultPopup
                property Component popupSource: Component {
                    MD.Popup {
                        readyForOpen: false
                        enter: null
                        exit: null
                    }
                }
                function launch() {
                    return MD.Util.showPopup(popupSource, {}, this)
                }
                property Component rejectedPopupSource: Component {
                    MD.Popup {
                        readyForOpen: false
                        Component.onCompleted: rejectOpen("rejected before requestOpen")
                    }
                }
                function launchRejected() {
                    return MD.Util.showPopup(rejectedPopupSource, {}, this)
                }
                MD.Popup {
                    id: popup
                    readyForOpen: false
                    enter: null
                    exit: null
                }
                MD.Popup {
                    id: defaultPopup
                    enter: null
                    exit: null
                }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/DeferredPopup.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(item);
        item->setParentItem(m_window.contentItem());

        auto* popup = qvariant_cast<QObject*>(object->property("popup"));
        QVERIFY(popup);
        QVERIFY(QMetaObject::invokeMethod(popup, "requestOpen"));
        QVERIFY(! popup->property("visible").toBool());
        QVERIFY(popup->property("openPending").toBool());
        QVERIFY(popup->setProperty("readyForOpen", true));
        QTRY_VERIFY(popup->property("visible").toBool());
        QVERIFY(! popup->property("openPending").toBool());
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));

        auto* defaultPopup = qvariant_cast<QObject*>(object->property("defaultPopup"));
        QVERIFY(defaultPopup);
        QVERIFY(defaultPopup->property("readyForOpen").toBool());
        QVERIFY(QMetaObject::invokeMethod(defaultPopup, "requestOpen"));
        QTRY_VERIFY(defaultPopup->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(defaultPopup, "close"));

        QVariant launched;
        QVERIFY(
            QMetaObject::invokeMethod(object.get(), "launch", Q_RETURN_ARG(QVariant, launched)));
        auto*             dynamicPopup = qvariant_cast<QObject*>(launched);
        QPointer<QObject> guarded      = dynamicPopup;
        QVERIFY(guarded);
        QVERIFY(guarded->property("openPending").toBool());
        QSignalSpy rejectionSpy(guarded, SIGNAL(openRejected(QString)));
        QVERIFY(QMetaObject::invokeMethod(
            guarded, "rejectOpen", Q_ARG(QVariant, QStringLiteral("rejected"))));
        QVERIFY(QMetaObject::invokeMethod(
            guarded, "rejectOpen", Q_ARG(QVariant, QStringLiteral("rejected again"))));
        QCOMPARE(rejectionSpy.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(guarded.isNull(), 2000);

        QVariant rejectedLaunch;
        QVERIFY(QMetaObject::invokeMethod(
            object.get(), "launchRejected", Q_RETURN_ARG(QVariant, rejectedLaunch)));
        QPointer<QObject> rejectedBeforeRequest = qvariant_cast<QObject*>(rejectedLaunch);
        QVERIFY(rejectedBeforeRequest);
        QTRY_VERIFY_WITH_TIMEOUT(rejectedBeforeRequest.isNull(), 2000);
    }

    void popupWithoutRequestOpenUsesOpen() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import QtQuick.Templates as T
            import Qcm.Material as MD

            Item {
                width: 640
                height: 480
                property Component popupSource: Component {
                    T.Popup {
                        enter: null
                        exit: null
                    }
                }
                function launch() {
                    return MD.Util.showPopup(popupSource, {}, this)
                }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/PlainPopup.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(item);
        item->setParentItem(m_window.contentItem());

        QTest::failOnWarning(QRegularExpression(QStringLiteral(".*requestOpen.*")));
        QVariant launched;
        QVERIFY(
            QMetaObject::invokeMethod(object.get(), "launch", Q_RETURN_ARG(QVariant, launched)));
        QPointer<QObject> popup = qvariant_cast<QObject*>(launched);
        QVERIFY(popup);
        QTRY_VERIFY(popup->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY_WITH_TIMEOUT(popup.isNull(), 2000);
    }

    void popupPresenterOpenAndClose() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentNormal");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        QVERIFY(presentation->active());
        m_engine.collectGarbage();
        QVERIFY(presentation->active());

        QSignalSpy closed(presentation, &qml_material::PopupPresentation::closed);
        presentation->close();
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Closed);
        QVERIFY(! presentation->active());
        QCOMPARE(closed.count(), 1);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterWaitsForAttachedReady() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentWaiting");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::WaitingForReady);

        QVERIFY(host->setProperty("popupReady", true));
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        presentation->close();
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Closed);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterCancelsAsynchronousCreationOnce() {
        auto host = createPresentationHost();
        QVERIFY(host);
        auto* presenter = qvariant_cast<qml_material::PopupPresenter*>(host->property("presenter"));
        QVERIFY(presenter);
        presenter->setIncubationMode(qml_material::Pool::Asynchronous);

        auto* presentation = invokePresentation(host.get(), "presentNormal");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Loading);
        QSignalSpy cancelled(presentation, &qml_material::PopupPresentation::cancelled);

        presentation->cancel();
        presentation->cancel();
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Cancelled);
        QCOMPARE(cancelled.count(), 1);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterReportsFailureAfterBinding() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentReporting");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::WaitingForReady);
        QSignalSpy failed(presentation, &qml_material::PopupPresentation::failed);

        QVERIFY(QMetaObject::invokeMethod(
            host.get(), "reportFailure", Q_ARG(QString, QStringLiteral("late failure"))));
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QCOMPARE(presentation->errorString(), QStringLiteral("late failure"));
        QCOMPARE(failed.count(), 1);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterClosesBeforeReportingVisibleFailure() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentOpenReporting");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        QSignalSpy failed(presentation, &qml_material::PopupPresentation::failed);

        QVERIFY(QMetaObject::invokeMethod(
            host.get(), "reportFailure", Q_ARG(QString, QStringLiteral("visible failure"))));
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QCOMPARE(presentation->errorString(), QStringLiteral("visible failure"));
        QCOMPARE(failed.count(), 1);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterReplaysEarlyFailure() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentFailed");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QCOMPARE(presentation->errorString(), QStringLiteral("early failure"));
        QVERIFY(! presentation->active());
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterCompletesWithResult() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentResult");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        QSignalSpy completed(presentation, &qml_material::PopupPresentation::completed);

        QVERIFY(QMetaObject::invokeMethod(
            host.get(), "submit", Q_ARG(QVariant, QStringLiteral("accepted"))));
        QCOMPARE(completed.count(), 1);
        QCOMPARE(presentation->result().toString(), QStringLiteral("accepted"));
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Closed);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterObservesExternalCloseOnce() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentControlled");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        QSignalSpy closed(presentation, &qml_material::PopupPresentation::closed);

        QVERIFY(QMetaObject::invokeMethod(host.get(), "dismiss"));
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Closed);
        QCOMPARE(closed.count(), 1);
        presentation->close();
        QCOMPARE(closed.count(), 1);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterReportsExternalDestruction() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentControlled");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        QSignalSpy failed(presentation, &qml_material::PopupPresentation::failed);

        auto* popup = qvariant_cast<QObject*>(host->property("controlledPopup"));
        QVERIFY(popup);
        delete popup;
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QVERIFY(presentation->errorString().contains(QStringLiteral("destroyed")));
        QCOMPARE(failed.count(), 1);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterRejectsNonPopupObject() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentInvalid");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QVERIFY(presentation->errorString().contains(QStringLiteral("popup contract")));
        QVERIFY(presentation->errorString().contains(QStringLiteral("open()")));
        QVERIFY(presentation->errorString().contains(QStringLiteral("close()")));
        QVERIFY(presentation->errorString().contains(QStringLiteral("opened()")));
        QVERIFY(presentation->errorString().contains(QStringLiteral("closed()")));
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterPreservesPoolErrors() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* missing = invokePresentation(host.get(), "presentMissing");
        QVERIFY(missing);
        QCOMPARE(missing->status(), qml_material::PopupPresentation::Error);
        QVERIFY(! missing->errorString().isEmpty());

        auto* required = invokePresentation(host.get(), "presentRequired");
        QVERIFY(required);
        QCOMPARE(required->status(), qml_material::PopupPresentation::Error);
        QVERIFY(required->errorString().contains(QStringLiteral("missingValue")));
        processDeferredDeletes();
        delete missing;
        delete required;
    }

    void popupPresenterResolvesRelativeSource() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile file(directory.filePath(QStringLiteral("RelativePopup.qml")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.Popup {
                enter: null
                exit: null
            }
        )") > 0);
        file.close();

        auto host = createRelativePresentationHost(
            QUrl::fromLocalFile(directory.filePath(QStringLiteral("PresentationHost.qml"))));
        QVERIFY(host);
        auto* presentation = invokePresentation(host.get(), "presentRelative");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        presentation->close();
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Closed);
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterRejectsOwnedParentProperty() {
        auto host = createPresentationHost();
        QVERIFY(host);

        auto* presentation = invokePresentation(host.get(), "presentWithParent");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QVERIFY(presentation->errorString().contains(QStringLiteral("parent")));
        processDeferredDeletes();
        delete presentation;
    }

    void popupPresenterRequiresWindowHost() {
        qml_material::PopupPresenter presenter;
        auto*                        presentation = presenter.present({}, {});
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Error);
        QVERIFY(presentation->errorString().contains(QStringLiteral("host")));
        delete presentation;
    }

    void popupPresenterHostDestructionCleansActivePresentation() {
        auto host = createPresentationHost();
        QVERIFY(host);
        QPointer<qml_material::PopupPresentation> presentation =
            invokePresentation(host.get(), "presentNormal");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);

        host.reset();
        processDeferredDeletes();
        QVERIFY(presentation);
        QVERIFY(presentation->status() == qml_material::PopupPresentation::Closed ||
                presentation->status() == qml_material::PopupPresentation::Cancelled);
        QVERIFY(! presentation->active());
        delete presentation;
    }

    void popupPresenterCloseAllIsIsolatedPerWindow() {
        QQuickWindow secondWindow;
        secondWindow.setGeometry(0, 0, 640, 480);
        secondWindow.create();

        auto firstHost  = createPresentationHost();
        auto secondHost = createPresentationHost();
        QVERIFY(firstHost);
        QVERIFY(secondHost);
        auto* secondItem = qobject_cast<QQuickItem*>(secondHost.get());
        QVERIFY(secondItem);
        secondItem->setParentItem(secondWindow.contentItem());

        auto* firstPresenter =
            qvariant_cast<qml_material::PopupPresenter*>(firstHost->property("presenter"));
        auto* secondPresenter =
            qvariant_cast<qml_material::PopupPresenter*>(secondHost->property("presenter"));
        QVERIFY(firstPresenter);
        QVERIFY(secondPresenter);

        auto* first  = invokePresentation(firstHost.get(), "presentNormal");
        auto* second = invokePresentation(secondHost.get(), "presentNormal");
        QVERIFY(first);
        QVERIFY(second);
        QCOMPARE(first->status(), qml_material::PopupPresentation::Open);
        QCOMPARE(second->status(), qml_material::PopupPresentation::Open);

        firstPresenter->closeAll();
        QCOMPARE(first->status(), qml_material::PopupPresentation::Closed);
        QCOMPARE(second->status(), qml_material::PopupPresentation::Open);
        secondPresenter->closeAll();
        QCOMPARE(second->status(), qml_material::PopupPresentation::Closed);
        processDeferredDeletes();
        delete first;
        delete second;
    }

    void popupPresenterKeepsActiveAndReferencedTerminalHandle() {
        auto host = createPresentationHost();
        QVERIFY(host);

        QPointer<qml_material::PopupPresentation> presentation =
            invokePresentation(host.get(), "holdNormal");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        m_engine.collectGarbage();
        QVERIFY(presentation);

        presentation->close();
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Closed);
        m_engine.collectGarbage();
        QVERIFY(presentation);

        QVERIFY(QMetaObject::invokeMethod(host.get(), "clearHeld"));
        QTRY_VERIFY_WITH_TIMEOUT(presentation.isNull(), 2000);
        processDeferredDeletes();
    }

private:
    QQmlEngine               m_engine;
    QQmlIncubationController m_controller;
    QQuickWindow             m_window;
};

QTEST_MAIN(DynamicCreationTest)

#include "dynamic_creation.moc"
