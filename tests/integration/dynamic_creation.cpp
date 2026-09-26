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
#include "qml_material/control/action_group.hpp"
#include "qml_material/control/application_window.hpp"
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/tool_separator.hpp"
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
    void popupSurvivesWindowDestruction() {
        auto window = std::make_unique<QQuickWindow>();
        window->resize(640, 480);
        QQuickItem host;
        host.setParentItem(window->contentItem());
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.PopupBase {
                width: 100; height: 80
                focus: true
                modal: true
            }
        )",
                          QUrl("qrc:/tests/popup-window-destruction.qml"));
        std::unique_ptr<qml_material::Popup> popup(
            qobject_cast<qml_material::Popup*>(component.create()));
        QVERIFY2(popup, qPrintable(component.errorString()));
        popup->setParentItem(&host);
        popup->open();
        QVERIFY(popup->isOpened());
        QCOMPARE(window->activeFocusItem(), popup->surfaceItem());
        QSignalSpy closed(popup.get(), &qml_material::Popup::closed);
        window.reset();
        QCOMPARE(closed.count(), 1);
        QVERIFY(! popup->isVisible());
        QVERIFY(! popup->overlayItem());
        QVERIFY(! popup->surfaceItem()->window());
        QVERIFY(! popup->surfaceItem()->parentItem());

        QQuickWindow replacement;
        host.setParentItem(replacement.contentItem());
        popup->open();
        QVERIFY(popup->isOpened());
        QCOMPARE(popup->surfaceItem()->window(), &replacement);
        popup->close();
    }

    void popupPresentedAboveBottomSheet_data() {
        QTest::addColumn<bool>("modal");
        QTest::newRow("modal-popup") << true;
        QTest::newRow("modeless-popup") << false;
    }

    void popupPresentedAboveBottomSheet() {
        QFETCH(bool, modal);
        QQuickWindow window;
        window.resize(640, 480);
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 640; height: 480
                property int infoClicks: 0
                property var infoPopup: null
                property var presentation: null
                property alias sheet: sheet
                MD.PopupPresenter {
                    id: presenter
                    host: root
                    incubationMode: MD.Pool.Synchronous
                }
                MD.BottomSheet {
                    id: sheet
                    parent: root
                    preferredContentHeight: 300
                    animationDuration: 0
                }
                Component {
                    id: infoSource
                    MD.Popup {
                        parent: root
                        x: 220; y: 240
                        width: 200; height: 120
                        focus: true
                        enter: null
                        exit: null
                        Component.onCompleted: root.infoPopup = this
                        MouseArea {
                            anchors.fill: parent
                            onClicked: ++root.infoClicks
                        }
                    }
                }
                function openInfo(modal) {
                    presentation = presenter.present(infoSource, {modal: modal})
                }
            }
        )",
                          QUrl("qrc:/tests/popup-above-sheet.qml"));
        std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(root, qPrintable(component.errorString()));
        root->setParentItem(window.contentItem());
        window.show();
        auto* sheet = root->property("sheet").value<qml_material::Popup*>();
        QVERIFY(sheet);
        sheet->open();
        QTRY_VERIFY(sheet->isOpened());
        for (int i = 0; i < 2; ++i) {
            QVERIFY(QMetaObject::invokeMethod(root.get(), "openInfo", Q_ARG(QVariant, modal)));
            auto* info = root->property("infoPopup").value<qml_material::Popup*>();
            QVERIFY(info);
            QTRY_VERIFY(info->isOpened());
            QCOMPARE(info->overlayItem(), sheet->overlayItem());
            QVERIFY(info->surfaceItem()->z() > sheet->surfaceItem()->z());
            QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(320, 300));
            QCOMPARE(root->property("infoClicks").toInt(), i + 1);
            QTest::keyClick(&window, Qt::Key_Escape);
            QTRY_VERIFY(! info->isVisible());
            QVERIFY(sheet->isOpened());
        }
        sheet->close();
        QTRY_VERIFY(! sheet->isVisible());
    }

    void popupCoordinates() {
        QQuickWindow window;
        window.resize(640, 480);
        QQuickItem host(window.contentItem());
        host.setPosition({ 40, 60 });
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.PopupBase {
                width: 100; height: 80
                x: 20; y: 30
                collisionPolicy: MD.PopupBase.Unrestricted
            }
        )",
                          QUrl("qrc:/tests/popup-coordinates.qml"));
        std::unique_ptr<qml_material::Popup> popup(
            qobject_cast<qml_material::Popup*>(component.create()));
        QVERIFY2(popup, qPrintable(component.errorString()));
        popup->setParentItem(&host);
        popup->open();
        QCOMPARE(popup->surfaceItem()->position(), QPointF(60, 90));
        popup->setPositioningItem(popup->overlayItem());
        QCOMPARE(popup->surfaceItem()->position(), QPointF(20, 30));
        QVERIFY(popup->isOpened());
        QCOMPARE(popup->parentItem(), &host);
        host.setPosition({ 80, 100 });
        QCOMPARE(popup->surfaceItem()->position(), QPointF(20, 30));
        popup->setX(-25);
        QCOMPARE(popup->surfaceItem()->x(), -25);
        popup->setCollisionPolicy(qml_material::Popup::Clamp);
        QCOMPARE(popup->surfaceItem()->x(), 0);
        popup->setCollisionPolicy(qml_material::Popup::Unrestricted);

        QQuickItem ancestor(window.contentItem());
        ancestor.setPosition({ 100, 110 });
        auto reference = std::make_unique<QQuickItem>(&ancestor);
        reference->setPosition({ 10, 15 });
        popup->setPositioningItem(reference.get());
        QCOMPARE(popup->surfaceItem()->position(),
                 reference->mapToItem(popup->overlayItem(), { -25, 30 }));
        ancestor.setX(200);
        ancestor.setRotation(30);
        QCOMPARE(popup->surfaceItem()->position(),
                 reference->mapToItem(popup->overlayItem(), { -25, 30 }));
        reference->setParentItem(&host);
        QCOMPARE(popup->surfaceItem()->position(),
                 reference->mapToItem(popup->overlayItem(), { -25, 30 }));
        QQuickWindow otherWindow;
        const auto   previous = popup->surfaceItem()->position();
        reference->setParentItem(otherWindow.contentItem());
        QCOMPARE(popup->surfaceItem()->position(), previous);
        reference->setParentItem(&host);
        QCOMPARE(popup->surfaceItem()->position(),
                 reference->mapToItem(popup->overlayItem(), { -25, 30 }));
        reference.reset();
        QVERIFY(! popup->positioningItem());
        QCOMPARE(popup->surfaceItem()->position(), QPointF(55, 130));
        popup->close();
        popup->open();
        QCOMPARE(popup->surfaceItem()->position(), QPointF(55, 130));
        host.setVisible(false);
        QVERIFY(! popup->isVisible());
    }

    void popupCoordinateAnimation() {
        QQuickWindow window;
        window.resize(640, 480);
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.PopupBase {
                positioningItem: overlayItem
                width: 100; height: 80
                property real offset: 0
                x: (overlayWidth - width) / 2 + offset
                y: (overlayHeight - height) / 2 + offset
                collisionPolicy: MD.PopupBase.Unrestricted
                Behavior on x { NumberAnimation { duration: 160 } }
                Behavior on y { NumberAnimation { duration: 160 } }
            }
        )",
                          QUrl("qrc:/tests/popup-coordinate-animation.qml"));
        std::unique_ptr<qml_material::Popup> popup(
            qobject_cast<qml_material::Popup*>(component.create()));
        QVERIFY2(popup, qPrintable(component.errorString()));
        popup->setParentItem(window.contentItem());
        popup->open();
        QTRY_COMPARE(popup->surfaceItem()->position(), QPointF(270, 200));
        popup->setProperty("offset", -400);
        QTRY_VERIFY(popup->x() < 270 && popup->x() > -130);
        QCOMPARE(popup->surfaceItem()->x(), popup->x());
        QCOMPARE(popup->surfaceItem()->y(), popup->y());
        QTRY_COMPARE(popup->surfaceItem()->position(), QPointF(-130, -200));
        popup->setWidth(200);
        QTRY_COMPARE(popup->surfaceItem()->x(), -180);
        window.resize(800, 600);
        QTRY_COMPARE(popup->surfaceItem()->position(), QPointF(-100, -140));
    }

    void bottomSheetCoordinates_data() {
        QTest::addColumn<bool>("modal");
        QTest::newRow("modal") << true;
        QTest::newRow("standard") << false;
    }
    void bottomSheetCoordinates() {
        QFETCH(bool, modal);
        QQuickWindow window;
        window.resize(640, 480);
        QQuickItem host(window.contentItem());
        host.setPosition({ 40, 60 });
        host.setSize({ 400, 300 });
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.BottomSheet { Item { width: 200; height: 100 } }
        )",
                          QUrl("qrc:/tests/bottom-sheet-coordinates.qml"));
        std::unique_ptr<qml_material::Popup> popup(
            qobject_cast<qml_material::Popup*>(component.create()));
        QVERIFY2(popup, qPrintable(component.errorString()));
        popup->setParentItem(&host);
        popup->setModal(modal);
        popup->open();
        QCOMPARE(popup->surfaceItem()->position(), modal ? QPointF(0, 0) : QPointF(40, 60));
        QCOMPARE(popup->surfaceItem()->size(), modal ? QSizeF(640, 480) : QSizeF(400, 300));
        popup->setX(25);
        popup->setY(-20);
        QCOMPARE(popup->surfaceItem()->position(), modal ? QPointF(25, -20) : QPointF(65, 40));
        host.setSize({ 500, 350 });
        QCOMPARE(popup->surfaceItem()->size(), modal ? QSizeF(640, 480) : QSizeF(500, 350));
    }

    void tooltipParentPressClosesModal_data() {
        QTest::addColumn<bool>("releaseOutside");
        QTest::newRow("inside-tooltip-parent") << false;
        QTest::newRow("tooltip-awaits-release") << true;
    }
    void tooltipParentPressClosesModal() {
        QFETCH(bool, releaseOutside);
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 640; height: 480
                property int clicks: 0
                MouseArea { anchors.fill: parent; onClicked: parent.clicks++ }
                Item {
                    x: 10; y: 10; width: 80; height: 80
                    MD.PlainToolTip {
                        objectName: "tip"; text: "Tip"; delay: 0; timeout: -1
                        enter: null; exit: null
                    }
                }
                MD.PopupBase {
                    objectName: "popup"
                    x: 220; y: 160; width: 200; height: 120
                    modal: true; dim: false
                    enter: null; exit: null
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        auto* popup = root->findChild<qml_material::Popup*>("popup");
        auto* tip   = root->findChild<qml_material::Popup*>("tip");
        QVERIFY(popup && tip);
        if (releaseOutside) tip->setClosePolicy(qml_material::Popup::CloseOnReleaseOutside);
        popup->open();
        tip->open();
        QTRY_VERIFY(popup->isOpened() && tip->isOpened());
        QSignalSpy closed(popup, &qml_material::Popup::closed);
        QTest::mousePress(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(40, 60));
        QCOMPARE(closed.size(), 1);
        QVERIFY(tip->isVisible());
        QTest::mouseRelease(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(40, 60));
        QCOMPARE(root->property("clicks").toInt(), 0);
        QCOMPARE(tip->isVisible(), ! releaseOutside);
    }

    void popupPressReentry() {
        qml_material::Popup popup;
        popup.setParentItem(m_window.contentItem());
        popup.setX(220);
        popup.setY(160);
        popup.setWidth(200);
        popup.setHeight(120);
        popup.setModal(true);
        popup.setClosePolicy(qml_material::Popup::CloseOnPressOutside |
                             qml_material::Popup::CloseOnReleaseOutside);
        static_cast<QQmlParserStatus*>(&popup)->componentComplete();
        popup.open();
        QSignalSpy closed(&popup, &qml_material::Popup::closed);
        const auto reopen = connect(&popup, &qml_material::Popup::closed, &popup, [&] {
            popup.open();
        });
        QTest::mousePress(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(40, 60));
        QCOMPARE(closed.size(), 1);
        QVERIFY(popup.isOpened());
        QTest::mouseRelease(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(40, 60));
        QCOMPARE(closed.size(), 1);
        QVERIFY(popup.isOpened());
        disconnect(reopen);
        popup.close();
    }

    void modalHover_data() {
        QTest::addColumn<bool>("qt");
        QTest::addColumn<bool>("dim");
        QTest::addColumn<int>("delay");
        for (bool qt : { false, true })
            for (bool dim : { false, true })
                for (int delay : { 0, 250 })
                    QTest::newRow(qPrintable(
                        QString("%1-dim%2-delay%3").arg(qt ? "qt" : "md").arg(dim).arg(delay)))
                        << qt << dim << delay;
    }
    void modalHover() {
        if (QGuiApplication::platformName() == "offscreen" ||
            QGuiApplication::platformName() == "minimal")
            QSKIP("Requires window-system hover delivery");
        QFETCH(bool, qt);
        QFETCH(bool, dim);
        QFETCH(int, delay);
        QQmlComponent component(&m_engine);
        QByteArray    source = R"(
            import QtQuick
            import QtQuick.Controls as QC
            import Qcm.Material as MD
            Item {
                width: 640; height: 480
                property bool tooltipVisible: button.TIP.visible
                property bool innerTooltipVisible: inner.TIP.visible
                MouseArea {
                    objectName: "hoverArea"
                    x: 120; y: 10; width: 60; height: 80
                    hoverEnabled: true
                }
                Item {
                    x: 180; y: 10; width: 60; height: 80
                    HoverHandler { objectName: "hoverHandler" }
                }
                TYPE.Button {
                    id: button; objectName: "button"
                    x: 10; y: 10; width: 100; height: 80
                    text: "Hover"; hoverEnabled: true
                    TIP.visible: hovered
                    TIP.text: "Tip"
                    TIP.delay: DELAY
                }
                TYPE.Popup {
                    objectName: "popup"
                    x: 220; y: 160; width: 200; height: 120
                    modal: true; dim: DIM
                    enter: null; exit: null
                    TYPE.Button {
                        id: inner; objectName: "inner"
                        width: 100; height: 60
                        text: "Inside"; hoverEnabled: true
                        TIP.visible: hovered
                        TIP.text: "Inside tip"
                        TIP.delay: 0
                    }
                }
            }
        )";
        source.replace("TYPE", qt ? "QC" : "MD");
        source.replace("TIP", qt ? "QC.ToolTip" : "MD.ToolTip");
        source.replace("DELAY", QByteArray::number(delay));
        source.replace("DIM", dim ? "true" : "false");
        component.setData(source, QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        auto* button = root->findChild<QQuickItem*>("button");
        auto* popup  = root->findChild<QObject*>("popup");
        QVERIFY(button && popup);
        m_window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&m_window));
        QTest::mouseMove(&m_window, QPoint(150, 100));
        QTest::mouseMove(&m_window, QPoint(50, 50));
        QTRY_VERIFY(button->property("hovered").toBool());
        if (delay == 0) QTRY_VERIFY(root->property("tooltipVisible").toBool());
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTest::qWait(500);
        qInfo() << "hover after modal" << button->property("hovered") << "tooltip"
                << root->property("tooltipVisible");
        if (qt) {
            if (dim) {
                QVERIFY(! button->property("hovered").toBool());
                QVERIFY(! root->property("tooltipVisible").toBool());
            }
            QVERIFY(QMetaObject::invokeMethod(popup, "close"));
            m_window.hide();
            return;
        }
        // Qt's dim=false behavior is a reference observation, not our chosen contract.
        if (! qt || dim) {
            QTRY_VERIFY(! button->property("hovered").toBool());
            QTRY_VERIFY(! root->property("tooltipVisible").toBool());
            QTest::mouseMove(&m_window, QPoint(150, 50));
            QTest::qWait(50);
            QVERIFY(! root->findChild<QObject*>("hoverArea")->property("containsMouse").toBool());
            QTest::mouseMove(&m_window, QPoint(210, 50));
            QTest::qWait(50);
            QVERIFY(! root->findChild<QObject*>("hoverHandler")->property("hovered").toBool());
        }
        auto* inner = root->findChild<QQuickItem*>("inner");
        QVERIFY(inner);
        QTest::mouseMove(&m_window, inner->mapToScene(QPointF(30, 30)).toPoint());
        QTRY_VERIFY(inner->property("hovered").toBool());
        QTRY_VERIFY(root->property("innerTooltipVisible").toBool());
        QTest::mouseMove(&m_window, QPoint(50, 50));
        if (! qt || dim) {
            QTest::qWait(300);
            QTRY_VERIFY(! button->property("hovered").toBool());
            QTRY_VERIFY(! root->property("tooltipVisible").toBool());
        }
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(button->property("hovered").toBool());
        popup->setProperty("modal", false);
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTest::qWait(300);
        QVERIFY(button->property("hovered").toBool());
        popup->setProperty("modal", true);
        QTRY_VERIFY(! button->property("hovered").toBool());
        popup->setProperty("deferredCompletion", true);
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTest::qWait(100);
        QVERIFY(! button->property("hovered").toBool());
        QVERIFY(QMetaObject::invokeMethod(popup, "completeExit"));
        QTRY_VERIFY(button->property("hovered").toBool());
        m_window.hide();
    }

    void tooltipOutsidePressDoesNotShieldModalPopup() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD

            Item {
                width: 640
                height: 480
                property alias popup: popup
                property alias tooltip: tooltip

                MD.PopupBase {
                    id: popup
                    x: 220
                    y: 160
                    width: 200
                    height: 120
                    modal: true
                    dim: false
                    collisionPolicy: MD.PopupBase.Unrestricted
                    closePolicy: MD.PopupBase.CloseOnEscape | MD.PopupBase.CloseOnPressOutside
                    enter: null
                    exit: null

                    Item {
                        id: tipAnchor
                        anchors.centerIn: parent
                        width: 48
                        height: 32

                        MD.PlainToolTip {
                            id: tooltip
                            parent: tipAnchor
                            text: "Tip"
                            delay: 0
                            timeout: -1
                            enter: null
                            exit: null
                        }
                    }
                }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/tooltip-outside-modal-popup.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        root->setParentItem(m_window.contentItem());

        auto* popup =
            qobject_cast<qml_material::Popup*>(qvariant_cast<QObject*>(object->property("popup")));
        auto* tooltip = qobject_cast<qml_material::Popup*>(
            qvariant_cast<QObject*>(object->property("tooltip")));
        QVERIFY(popup);
        QVERIFY(tooltip);

        popup->open();
        QTRY_VERIFY(popup->isOpened());
        tooltip->open();
        QTRY_VERIFY(tooltip->isOpened());

        QTest::mouseClick(&m_window, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
        QTRY_VERIFY(! tooltip->isVisible());
        QTRY_VERIFY(! popup->isVisible());
    }

    void applicationWindowOwnsContent() {
        QTest::failOnWarning(QRegularExpression(QStringLiteral(".*contentItem.*overrides.*")));
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD

            MD.ApplicationWindow {
                width: 320
                height: 240
                property alias contentChild: contentChild
                property alias appBackground: appBackground
                property alias appMenuBar: appMenuBar
                property alias appHeader: appHeader
                property alias appFooter: appFooter

                background: Item { id: appBackground }
                menuBar: Item { id: appMenuBar }
                header: Item { id: appHeader }
                footer: Item { id: appFooter }

                Item {
                    id: contentChild
                    objectName: "contentChild"
                    width: 16
                    height: 16
                }
            }
        )",
                          QUrl(QStringLiteral("qrc:/tests/application-window-content.qml")));
        QVERIFY2(! component.isError(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* window = qobject_cast<qml_material::ApplicationWindow*>(object.get());
        QVERIFY(window);

        auto* contentChild = qvariant_cast<QQuickItem*>(object->property("contentChild"));
        QVERIFY(contentChild);
        QCOMPARE(contentChild->parentItem(), window->contentItem());
        QCOMPARE(window->background(),
                 qvariant_cast<QQuickItem*>(object->property("appBackground")));
        QCOMPARE(window->menuBar(), qvariant_cast<QQuickItem*>(object->property("appMenuBar")));
        QCOMPARE(window->header(), qvariant_cast<QQuickItem*>(object->property("appHeader")));
        QCOMPARE(window->footer(), qvariant_cast<QQuickItem*>(object->property("appFooter")));
        QCOMPARE(window->frameItem()->parentItem(),
                 static_cast<QQuickWindow*>(window)->contentItem());
        QCOMPARE(window->frameItem()->size(),
                 static_cast<QQuickWindow*>(window)->contentItem()->size());
    }

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

    void nativeControlExports() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                MD.ActionGroup {
                    id: actions
                    objectName: "actions"
                    MD.Action { id: firstAction; checkable: true }
                }
                MD.Action {
                    id: secondAction
                    checkable: true
                    MD.ActionGroup.group: actions
                }
                MD.ButtonGroup { id: buttons; objectName: "buttons" }
                MD.ButtonBase {
                    id: firstButton
                    checkable: true
                    MD.ButtonGroup.group: buttons
                }
                MD.ButtonBase {
                    id: secondButton
                    checkable: true
                    MD.ButtonGroup.group: buttons
                }
                MD.ToolSeparator { id: separator; objectName: "separator" }
                function check() {
                    if (actions.actions.length !== 2 || buttons.buttons.length !== 2)
                        return "group membership";
                    firstAction.checked = true;
                    secondAction.checked = true;
                    if (firstAction.checked || actions.checkedAction !== secondAction)
                        return "action selection";
                    firstButton.checked = true;
                    secondButton.checked = true;
                    if (firstButton.checked || buttons.checkedButton !== secondButton)
                        return "button selection";
                    secondAction.MD.ActionGroup.group = null;
                    secondButton.MD.ButtonGroup.group = null;
                    if (actions.actions.length !== 1 || buttons.buttons.length !== 1)
                        return "detach";
                    if (!separator.vertical) return "default orientation";
                    separator.orientation = Qt.Horizontal;
                    if (!separator.horizontal || separator.vertical) return "orientation";
                    return "";
                }
            }
        )",
                          QUrl("qrc:/tests/NativeControlExports.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> host(component.create());
        QVERIFY(host);
        auto* actions   = host->findChild<qml_material::ActionGroup*>("actions");
        auto* buttons   = host->findChild<qml_material::ButtonGroup*>("buttons");
        auto* separator = host->findChild<qml_material::ToolSeparator*>("separator");
        QVERIFY(actions && buttons && separator);
        QCOMPARE(actions->metaObject(), &qml_material::ActionGroup::staticMetaObject);
        QCOMPARE(buttons->metaObject(), &qml_material::ButtonGroup::staticMetaObject);
        QCOMPARE(separator->metaObject(), &qml_material::ToolSeparator::staticMetaObject);
        QVariant result;
        QVERIFY(QMetaObject::invokeMethod(host.get(), "check", Q_RETURN_ARG(QVariant, result)));
        QCOMPARE(result.toString(), QString());
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

int run_dynamic_creation(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    DynamicCreationTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "dynamic_creation.moc"
