#include "qml_material/item/item_proxy.hpp"
#include "qml_material/control/popup.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtTest>
#include <memory>

using namespace qml_material;

class ItemProxyTest : public QObject {
    Q_OBJECT
private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*Binding loop.*")); }
    void ownershipAndGeometry() {
        QObject owner;
        auto*   target = new QQuickItem;
        target->setParent(&owner);
        target->setImplicitWidth(80);
        target->setImplicitHeight(90);
        QPointer<QQuickItem> alive(target);
        {
            ItemProxy first, second;
            first.setSize({ 200, 300 });
            first.setTarget(target);
            QCOMPARE(first.implicitWidth(), 80);
            first.setActive(true);
            QVERIFY(first.controlling());
            QCOMPARE(target->parent(), &owner);
            QCOMPARE(target->parentItem(), &first);
            QCOMPARE(target->size(), first.size());
            first.setVisible(false);
            QVERIFY(first.controlling());
            first.setActive(false);
            QVERIFY(! target->parentItem());
            second.setSize({ 320, 400 });
            second.setTarget(target);
            second.setActive(true);
            QCOMPARE(target->size(), second.size());
            target->setImplicitWidth(123);
            QCOMPARE(first.implicitWidth(), 123);
            QCOMPARE(second.implicitWidth(), 123);
            second.setTarget(nullptr);
            QVERIFY(alive);
            QCOMPARE(second.implicitWidth(), 0);
            second.setTarget(target);
        }
        QVERIFY(alive);
        QVERIFY(! target->parentItem());
    }
    void conflictAndDestruction() {
        ItemProxy first, second;
        auto      target = std::make_unique<QQuickItem>();
        first.setTarget(target.get());
        second.setTarget(target.get());
        first.setActive(true);
        QTest::ignoreMessage(
            QtWarningMsg,
            QRegularExpression(".*target is already controlled by another ItemProxy"));
        second.setActive(true);
        QVERIFY(first.controlling());
        QVERIFY(! second.controlling());
        first.setActive(false);
        QVERIFY(! second.controlling());
        second.setActive(false);
        second.setActive(true);
        QVERIFY(second.controlling());
        target.reset();
        QVERIFY(! first.target());
        QVERIFY(! second.target());
        QVERIFY(! second.controlling());
    }
    void reentry() {
        QQuickItem target, replacement;
        auto       proxy = std::make_unique<ItemProxy>();
        proxy->setTarget(&target);
        proxy->setActive(true);
        connect(proxy.get(), &ItemProxy::controllingChanged, &target, [&] {
            if (! proxy->controlling()) proxy->setTarget(&replacement);
        });
        proxy->setTarget(nullptr);
        QCOMPARE(proxy->target(), &replacement);
        QVERIFY(proxy->controlling());
        disconnect(proxy.get(), nullptr, &target, nullptr);
        connect(proxy.get(), &ItemProxy::controllingChanged, &target, [&] {
            proxy.reset();
        });
        proxy->setActive(false);
        QVERIFY(! proxy);
        QVERIFY(! replacement.parentItem());
    }
    void invalidTargetsAndExternalParent() {
        QQuickWindow firstWindow, secondWindow;
        QQuickItem   target(firstWindow.contentItem());
        ItemProxy    proxy(secondWindow.contentItem());
        proxy.setTarget(&target);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*different window"));
        proxy.setActive(true);
        QVERIFY(! proxy.controlling());
        proxy.setActive(false);
        proxy.setParentItem(&target);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*visual parent cycle"));
        proxy.setActive(true);
        QVERIFY(! proxy.controlling());
        proxy.setActive(false);
        proxy.setParentItem(firstWindow.contentItem());
        proxy.setActive(true);
        QVERIFY(proxy.controlling());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*reparented outside.*"));
        target.setParentItem(firstWindow.contentItem());
        QVERIFY(! proxy.controlling());
        QCOMPARE(target.parentItem(), firstWindow.contentItem());
    }
    void sheetViewportAndDismiss() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(500, 700);
        window.show();
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.BottomSheet {
    preferredContentHeight: 1000
    property int closes: 0
    property bool reopenOnce: false
    onClosed: {
        ++closes;
        if (reopenOnce) {
            reopenOnce = false;
            Qt.callLater(open);
        }
    }
    MD.ItemProxy {
        width: parent.width
        height: 0
    }
}
)",
                          QUrl());
        std::unique_ptr<Popup> sheet(qobject_cast<Popup*>(component.create()));
        QVERIFY2(sheet, qPrintable(component.errorString()));
        sheet->setParentItem(window.contentItem());
        sheet->open();
        QVERIFY(sheet->isVisible());
        QCOMPARE(sheet->property("contentViewportWidth").toReal(), 500);
        QCOMPARE(sheet->property("contentViewportHeight").toReal(), 580);
        QVERIFY(QMetaObject::invokeMethod(sheet.get(), "dismissImmediately"));
        QVERIFY(! sheet->isVisible());
        QCOMPARE(sheet->property("closes").toInt(), 1);
        QCOMPARE(sheet->property("_scrimOpacity").toReal(), 0);
        sheet->open();
        sheet->dismissImmediately();
        QCOMPARE(sheet->property("closes").toInt(), 2);
        sheet->setProperty("reopenOnce", true);
        sheet->open();
        sheet->dismissImmediately();
        QTRY_VERIFY(sheet->isOpened());
        QCOMPARE(sheet->property("closes").toInt(), 3);
        sheet->dismissImmediately();
        sheet->setProperty("preferredContentHeight", -1);
        QCOMPARE(sheet->property("contentViewportHeight").toReal(), 0);
    }
    void sameItemAcrossPaneAndSheet() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(900, 700);
        window.show();
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: root
    width: 900; height: 700
    property int creations: 0
    property int presses: 0
    property Item detail: Item {
        objectName: "detail"
        parent: null
        property int value: 42
        Component.onCompleted: ++root.creations
        MouseArea { objectName: "mouse"; anchors.fill: parent; onPressed: ++root.presses }
    }
    MD.SplitView {
        anchors.fill: parent
        MD.Pane { MD.SplitViewBase.fillWidth: true }
        MD.Pane {
            MD.SplitViewBase.minimumWidth: 280
            MD.SplitViewBase.maximumWidth: 280
            MD.SplitViewBase.preferredWidth: 280
            padding: 0
            contentItem: MD.ItemProxy {
                id: side
                objectName: "side"
                target: root.detail
                active: true
            }
        }
    }
    property MD.BottomSheet sheet: MD.BottomSheet {
        id: sheet
        parent: root
        preferredContentHeight: root.height
        MD.ItemProxy {
            id: floating
            objectName: "floating"
            width: sheet.contentViewportWidth
            height: sheet.contentViewportHeight
            target: root.detail
        }
    }
    function showSheet() {
        side.active = false;
        floating.active = true;
        sheet.open();
    }
    function showSide() {
        sheet.dismissImmediately();
        floating.active = false;
        side.active = true;
    }
}
)",
                          QUrl());
        std::unique_ptr<QQuickItem> root(qobject_cast<QQuickItem*>(component.create()));
        QVERIFY2(root, qPrintable(component.errorString()));
        root->setParentItem(window.contentItem());
        auto* detail = root->property("detail").value<QQuickItem*>();
        QVERIFY(detail);
        auto* owner = detail->parent();
        auto* mouse = detail->findChild<QQuickItem*>("mouse");
        QTRY_COMPARE(detail->width(), 280);
        QTest::mousePress(&window, Qt::LeftButton, {}, mouse->mapToScene({ 20, 20 }).toPoint());
        QCOMPARE(window.mouseGrabberItem(), mouse);
        QCOMPARE(root->property("presses").toInt(), 1);
        detail->forceActiveFocus();
        QVERIFY(QMetaObject::invokeMethod(root.get(), "showSheet"));
        QVERIFY(window.mouseGrabberItem() != mouse);
        QTest::mouseRelease(&window, Qt::LeftButton);
        auto* sheet = root->property("sheet").value<Popup*>();
        QVERIFY(sheet);
        QTRY_VERIFY(sheet->isOpened());
        QCOMPARE(detail->width(), sheet->property("contentViewportWidth").toReal());
        QCOMPARE(detail->height(), sheet->property("contentViewportHeight").toReal());
        QVERIFY(QMetaObject::invokeMethod(root.get(), "showSide"));
        QVERIFY(! sheet->isVisible());
        QTRY_COMPARE(detail->width(), 280);
        QCOMPARE(detail->parent(), owner);
        QCOMPARE(detail->property("value").toInt(), 42);
        QCOMPARE(root->property("creations").toInt(), 1);
        QTest::mouseClick(&window, Qt::LeftButton, {}, mouse->mapToScene({ 20, 20 }).toPoint());
        QCOMPARE(root->property("presses").toInt(), 2);
    }
};

int run_item_proxy(int argc, char** argv) {
    ItemProxyTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "item_proxy.moc"
