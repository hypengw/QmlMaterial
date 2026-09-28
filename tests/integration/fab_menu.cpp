#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/fab_menu.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/scrollable/flickable.hpp"

using namespace qml_material;

class FABMenuTest : public QObject {
    Q_OBJECT
private slots:
    void layoutAndInput_data() {
        QTest::addColumn<bool>("expanded");
        QTest::newRow("collapsed") << false;
        QTest::newRow("expanded") << true;
    }
    void layoutAndInput() {
        QFETCH(bool, expanded);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FABMenu {
    width: 300; height: 260
    animationsEnabled: false
    MD.FABMenuItem { text: "First"; icon.name: "stars"; animationsEnabled: false }
    MD.FABMenuItem { text: "Second"; icon.name: "stars"; animationsEnabled: false }
})",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(400, 300);
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "expanded", expanded } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* menu = qobject_cast<FABMenu*>(object.get());
        QVERIFY(menu);
        menu->setParentItem(window.contentItem());
        window.show();
        QTRY_VERIFY(menu->motionEnabled());
        QCOMPARE(menu->count(), 2);
        QCOMPARE(menu->visibleCount(), 2);
        QVERIFY(! menu->transitioning());
        auto* first  = qobject_cast<FABMenuItem*>(menu->itemAt(0));
        auto* second = qobject_cast<FABMenuItem*>(menu->itemAt(1));
        QVERIFY(first && second);
        QCOMPARE(first->height(), 56.);
        QCOMPARE(menu->button()->size(), QSizeF(56, 56));
        QCOMPARE(menu->button()->y(), 188.);
        QCOMPARE(first->inputEnabled(), expanded);
        QVERIFY(first->isVisible());
        QCOMPARE(first->alphaProgress(), expanded ? 1. : 0.);
        menu->open();
        QTRY_VERIFY(first->inputEnabled());
        QVERIFY(first->width() < second->width());
        QCOMPARE(first->mapToItem(menu, QPointF(first->width(), 0)).x(), 284.);
        QCOMPARE(second->mapToItem(menu, QPointF(second->width(), 0)).x(), 284.);
        QCOMPARE(second->mapToItem(menu, QPointF()).y() - first->mapToItem(menu, QPointF()).y(),
                 60.);
        QCOMPARE(menu->button()->y() - second->mapToItem(menu, QPointF(0, second->height())).y(),
                 8.);
        auto* viewport = menu->viewport();
        QVERIFY(viewport->contains(first->mapToItem(viewport, QPointF(first->width() / 2, 28))));
        QVERIFY(! viewport->contains(first->mapToItem(viewport, QPointF(first->width() / 2, 58))));
        menu->button()->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Tab);
        QCOMPARE(window.activeFocusItem(), first);
        QTest::keyClick(&window, Qt::Key_Up);
        QCOMPARE(window.activeFocusItem(), menu->button());
        first->forceActiveFocus();
        menu->close();
        QVERIFY(! first->inputEnabled());
        QCOMPARE(window.activeFocusItem(), menu->button());
        QTRY_COMPARE(first->alphaProgress(), 0.);
        menu->open();
        QTRY_COMPARE(first->alphaProgress(), 1.);
        menu->setLayoutDirection(Qt::RightToLeft);
        QTRY_COMPARE(first->mapToItem(menu, QPointF()).x(), 16.);
        QCOMPARE(second->mapToItem(menu, QPointF()).x(), 16.);
        QCOMPARE(menu->button()->x(), 16.);
    }
    void constrainedAndDynamic() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FABMenu {
    width: 200; height: 180; expanded: true; animationsEnabled: false
    property bool showFirst: true
    MD.FABMenuItem { text: "A very long menu label"; visible: parentMenu.showFirst; animationsEnabled: false }
    MD.FABMenuItem { text: "Second"; animationsEnabled: false }
    MD.FABMenuItem { text: "Third"; animationsEnabled: false }
    id: parentMenu
})",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow             window;
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* menu = qobject_cast<FABMenu*>(object.get());
        menu->setParentItem(window.contentItem());
        window.show();
        QTRY_VERIFY(menu->motionEnabled());
        auto* scroll = qobject_cast<Flickable*>(menu->viewport());
        QVERIFY(scroll);
        QVERIFY(scroll->isInteractive());
        QCOMPARE(scroll->height(), 100.);
        auto* first = qobject_cast<FABMenuItem*>(menu->itemAt(0));
        QCOMPARE(first->width(), 168.);
        menu->close();
        QTRY_COMPARE(first->alphaProgress(), 0.);
        menu->setProperty("showFirst", false);
        menu->open();
        QTRY_COMPARE(menu->visibleCount(), 2);
        QVERIFY(! first->isVisible());
        menu->setProperty("showFirst", true);
        QTRY_COMPARE(menu->visibleCount(), 3);
        QVERIFY(first->isVisible());
        delete menu->itemAt(1);
        QTRY_COMPARE(menu->count(), 2);
        QTRY_COMPARE(menu->visibleCount(), 2);
        QCOMPARE(menu->button()->y(), 108.);
        menu->setHeight(400);
        QTRY_VERIFY(! scroll->isInteractive());
    }
    void exampleLoads() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine,
                                QUrl::fromLocalFile(QFINDTESTDATA("../../example/FABMenus.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto menus = object->findChildren<FABMenu*>();
        QCOMPARE(menus.size(), 2);
        QCOMPARE(menus[1]->count(), 6);
    }
    void transitionsAndAction() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FABMenu {
    id: menu
    width: 300; height: 260; expanded: true
    property int calls: 0
    MD.FABMenuItem {
        text: "First"
        action: MD.Action { onTriggered: menu.calls++ }
    }
    MD.FABMenuItem { text: "Second" }
})",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(400, 300);
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* menu = qobject_cast<FABMenu*>(object.get());
        menu->setParentItem(window.contentItem());
        window.show();
        QTRY_VERIFY(menu->motionEnabled());
        QVERIFY(! menu->transitioning());
        auto* first  = qobject_cast<FABMenuItem*>(menu->itemAt(0));
        auto* second = qobject_cast<FABMenuItem*>(menu->itemAt(1));
        QCOMPARE(first->widthProgress(), 1.);
        QCOMPARE(menu->button()->property("checkedProgress").toReal(), 1.);
        QTest::mouseClick(&window,
                          Qt::LeftButton,
                          {},
                          first->mapToScene(QPointF(first->width() / 2, 28)).toPoint());
        QCOMPARE(menu->property("calls").toInt(), 1);
        QVERIFY(menu->expanded());
        first->setHoverEnabled(true);
        QHoverEvent hover(QEvent::HoverEnter, QPointF(10, 10), QPointF(10, 10), QPointF());
        QCoreApplication::sendEvent(first, &hover);
        QVERIFY(first->hovered());
        QTest::mousePress(&window,
                          Qt::LeftButton,
                          {},
                          first->mapToScene(QPointF(first->width() / 2, 28)).toPoint());
        QVERIFY(first->isPressed());
        menu->close();
        QVERIFY(! first->inputEnabled());
        QVERIFY(! first->hovered());
        QVERIFY(! first->isPressed());
        QTest::mouseRelease(&window,
                            Qt::LeftButton,
                            {},
                            first->mapToScene(QPointF(first->width() / 2, 28)).toPoint());
        QCOMPARE(menu->property("calls").toInt(), 1);
        QTRY_VERIFY(menu->transitioning());
        menu->open();
        QTRY_VERIFY(! menu->transitioning());
        QTRY_COMPARE(first->widthProgress(), 1.);
        QCOMPARE(second->alphaProgress(), 1.);
        menu->close();
        QTRY_VERIFY(! menu->transitioning());
        QTRY_COMPARE(first->alphaProgress(), 0.);
        QCOMPARE(second->alphaProgress(), 0.);
        QVERIFY(! first->inputEnabled());
        menu->setAnimationsEnabled(false);
        menu->open();
        QTRY_COMPARE(first->widthProgress(), 1.);
        menu->setRevealCount(.6);
        QTRY_VERIFY(! first->revealed() && second->revealed());
        menu->setRevealCount(0);
        QTRY_VERIFY(! second->revealed());
        menu->close();
        menu->open();
        QTRY_VERIFY(first->revealed());
        connect(first, &FABMenuItem::fullWidthChanged, menu, [first] {
            delete first;
        });
        menu->setWidth(60);
        QTRY_COMPARE(menu->count(), 1);
    }
};
int run_fab_menu(int argc, char** argv) {
    FABMenuTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "fab_menu.moc"
