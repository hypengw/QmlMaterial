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

#include "qml_material/control/popup.hpp"
#include "qml_material/control/panel.hpp"

namespace
{

void processDeferredDeletes() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

} // namespace

class PopupLifecycleTest : public QObject {
    Q_OBJECT

private slots:
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
                    readonly property bool testHovered: HOVER
                    TIP.visible: testHovered
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
                        readonly property bool testHovered: HOVER
                    TIP.visible: testHovered
                        TIP.text: "Inside tip"
                        TIP.delay: 0
                    }
                }
            }
        )";
        source.replace("HOVER", qt ? "hovered" : "mdState.interactions.hovered");
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
        QTRY_VERIFY(button->property("testHovered").toBool());
        if (delay == 0) QTRY_VERIFY(root->property("tooltipVisible").toBool());
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTest::qWait(500);
        qInfo() << "hover after modal" << button->property("testHovered") << "tooltip"
                << root->property("tooltipVisible");
        if (qt) {
            if (dim) {
                QVERIFY(! button->property("testHovered").toBool());
                QVERIFY(! root->property("tooltipVisible").toBool());
            }
            QVERIFY(QMetaObject::invokeMethod(popup, "close"));
            m_window.hide();
            return;
        }
        // Qt's dim=false behavior is a reference observation, not our chosen contract.
        if (! qt || dim) {
            QTRY_VERIFY(! button->property("testHovered").toBool());
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
        QTRY_VERIFY(inner->property("testHovered").toBool());
        QTRY_VERIFY(root->property("innerTooltipVisible").toBool());
        QTest::mouseMove(&m_window, QPoint(50, 50));
        if (! qt || dim) {
            QTest::qWait(300);
            QTRY_VERIFY(! button->property("testHovered").toBool());
            QTRY_VERIFY(! root->property("tooltipVisible").toBool());
        }
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(button->property("testHovered").toBool());
        popup->setProperty("modal", false);
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTest::qWait(300);
        QVERIFY(button->property("testHovered").toBool());
        popup->setProperty("modal", true);
        QTRY_VERIFY(! button->property("testHovered").toBool());
        popup->setProperty("deferredCompletion", true);
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTest::qWait(100);
        QVERIFY(! button->property("testHovered").toBool());
        QVERIFY(QMetaObject::invokeMethod(popup, "completeExit"));
        QTRY_VERIFY(button->property("testHovered").toBool());
        m_window.hide();
    }

    void popupHoverHandlerToolTip() {
        if (QGuiApplication::platformName() == "offscreen" ||
            QGuiApplication::platformName() == "minimal")
            QSKIP("Requires window-system hover delivery");
        QQmlComponent component(&m_engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 640; height: 480
                property var tip: tag.MD.ToolTip.toolTip
                Item {
                    x: 10; y: 10; width: 80; height: 40
                    HoverHandler { id: outside; objectName: "outside" }
                    readonly property bool testHovered: mdState.interactions.hovered
                    MD.ToolTip.visible: testHovered
                    MD.ToolTip.text: "Outside popup"
                    MD.ToolTip.delay: 0
                }
                MD.Popup {
                    objectName: "popup"
                    x: 150; y: 100; width: 300; height: 240
                    modal: true
                    enter: null; exit: null
                    Rectangle {
                        id: tag; objectName: "tag"
                        x: 40; y: 60; width: 100; height: 40
                        HoverHandler { id: hover; objectName: "hover" }
                        MD.ToolTip.visible: hover.hovered
                        MD.ToolTip.text: "Inside popup"
                        MD.ToolTip.delay: 0
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(m_window.contentItem());
        auto* popup   = root->findChild<qml_material::Popup*>("popup");
        auto* tag     = root->findChild<QQuickItem*>("tag");
        auto* hover   = root->findChild<QObject*>("hover");
        auto* outside = root->findChild<QObject*>("outside");
        auto* tip     = qobject_cast<qml_material::Popup*>(root->property("tip").value<QObject*>());
        QVERIFY(popup && tag && hover && outside && tip);
        m_window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&m_window));
        QTest::mouseMove(&m_window, QPoint(30, 30));
        QTRY_VERIFY(outside->property("testHovered").toBool());
        QTRY_VERIFY(tip->isOpened());
        popup->open();
        QTRY_VERIFY(! outside->property("testHovered").toBool());
        QTest::mouseMove(&m_window, tag->mapToScene(QPointF(30, 20)).toPoint());
        QTRY_VERIFY(hover->property("hovered").toBool());
        QTRY_VERIFY(tip->isOpened());
        QVERIFY(tip->surfaceItem()->z() > popup->surfaceItem()->z());
        popup->setZ(20);
        QTRY_VERIFY(hover->property("hovered").toBool());
        QVERIFY(tip->surfaceItem()->z() > popup->surfaceItem()->z());
        QCOMPARE(tip->z(), popup->z());
        tip->setZ(0);
        QVERIFY(tip->surfaceItem()->z() < popup->surfaceItem()->z());
        tip->resetZ();
        QVERIFY(tip->surfaceItem()->z() > popup->surfaceItem()->z());
        popup->setZ(5);
        QCOMPARE(tip->z(), popup->z());
        QVERIFY(tip->surfaceItem()->z() > popup->surfaceItem()->z());

        qml_material::Popup cover;
        cover.setParentItem(root);
        cover.setZ(popup->z());
        const auto tagPosition = tag->mapToScene(QPointF());
        cover.setX(tagPosition.x() + 20);
        cover.setY(tagPosition.y());
        cover.setWidth(80);
        cover.setHeight(40);
        static_cast<QQmlParserStatus*>(&cover)->componentComplete();
        cover.open();
        QVERIFY(cover.isOpened());
        QTRY_VERIFY(! hover->property("hovered").toBool());
        QTRY_VERIFY(! tip->isVisible());
        QTest::mouseMove(&m_window, tag->mapToScene(QPointF(10, 20)).toPoint());
        QTRY_VERIFY(hover->property("hovered").toBool());
        QTRY_VERIFY(tip->isOpened());
        cover.close();
        popup->close();
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

    void initTestCase() {
        m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        m_engine.setIncubationController(&m_controller);
        m_window.setGeometry(0, 0, 640, 480);
        m_window.create();
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

private:
    QQmlEngine               m_engine;
    QQmlIncubationController m_controller;
    QQuickWindow             m_window;
};

int run_popup_lifecycle(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    PopupLifecycleTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "popup_lifecycle.moc"
