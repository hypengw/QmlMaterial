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
#include "qml_material/control/popup.hpp"
#include "qml_material/util/pool.hpp"

namespace
{

void processDeferredDeletes() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

} // namespace

class PopupPresenterTest : public QObject {
    Q_OBJECT

private:
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

private slots:
    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_engine.setIncubationController(&m_controller);
        m_window.setGeometry(0, 0, 640, 480);
        m_window.create();
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

    void popupPresenterOwnsPresentationLease() {
        auto host = createPresentationHost();
        QVERIFY(host);
        auto* presentation = invokePresentation(host.get(), "presentControlled");
        QVERIFY(presentation);
        QCOMPARE(presentation->status(), qml_material::PopupPresentation::Open);
        auto* popup =
            qobject_cast<qml_material::Popup*>(host->property("controlledPopup").value<QObject*>());
        QVERIFY(popup);
        QCOMPARE(popup->presentationOwner(), presentation);
        QObject competitor;
        QVERIFY(! popup->acquirePresentation(&competitor));
        QVERIFY(popup->isOpened());
        presentation->close();
        QCOMPARE(popup->presentationOwner(), nullptr);
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

int run_popup_presenter(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    PopupPresenterTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "popup_presenter.moc"
