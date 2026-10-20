#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/fab_menu.hpp"
#include "qml_material/control/action.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/toggle_button.hpp"
#include "qml_material/scrollable/flickable.hpp"
#include "../input_test.hpp"

using namespace qml_material;
namespace
{
std::unique_ptr<CommandButton> commandFor(int kind) {
    if (kind) return std::make_unique<FABMenuItem>();
    return std::make_unique<CommandButton>();
}
} // namespace

class FABMenuTest : public QObject {
    Q_OBJECT
private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }
    void commandInterfaces() {
        for (int kind = 0; kind < 2; ++kind) {
            auto control = commandFor(kind);
            QCOMPARE((control.get())->metaObject()->indexOfProperty("checkable"), -1);
            for (const char* name :
                 { "action", "checked", "checkable", "selected", "pressX", "pressY", "pressed" })
                QCOMPARE(control->metaObject()->indexOfProperty(name), -1);
            QCOMPARE(control->metaObject()->indexOfSignal("toggled()"), -1);
            QVERIFY(control->metaObject()->indexOfSignal("clicked()") >= 0);
        }
    }
    void commandInput_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("input");
        for (int kind = 0; kind < 2; ++kind)
            for (int input = 0; input < 4; ++input)
                QTest::newRow(qPrintable(QString("%1-%2").arg(kind).arg(input))) << kind << input;
    }
    void commandInput() {
        QFETCH(int, kind);
        QFETCH(int, input);
        QQuickWindow window;
        auto         control = commandFor(kind);
        control->setParentItem(window.contentItem());
        control->setSize({ 100, 50 });
        window.resize(200, 100);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        QSignalSpy clicks(control.get(), &CommandButton::clicked);
        const auto activate = [&] {
            if (input == 0)
                control->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
            else if (input == 2) {
                control->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
                QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
            }
            flushInteractions();
        };
        activate();
        activate();
        QCOMPARE(clicks.size(), 2);
        QVERIFY(! feedback.values().pressed);
        control->setEnabled(false);
        activate();
        QCOMPARE(clicks.size(), 2);
    }
    void commandCancelled_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("change");
        for (int kind = 0; kind < 2; ++kind)
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(kind).arg(change))) << kind << change;
    }
    void commandCancelled() {
        QFETCH(int, kind);
        QFETCH(int, change);
        auto control = commandFor(kind);
        control->setSize({ 100, 50 });
        QSignalSpy             clicks(control.get(), &CommandButton::clicked);
        InteractionAccumulator feedback(this);
        feedback.setSource(control->interactionSource());
        QMouseEvent press(QEvent::MouseButtonPress,
                          { 20, 20 },
                          { 20, 20 },
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &press);
        flushInteractions();
        QVERIFY(feedback.values().pressed);
        InteractionSource next;
        if (change == 0)
            control->setEnabled(false);
        else if (change == 1)
            control->setVisible(false);
        else
            control->setInteractionSource(&next);
        QMouseEvent release(QEvent::MouseButtonRelease,
                            { 20, 20 },
                            { 20, 20 },
                            Qt::LeftButton,
                            Qt::NoButton,
                            Qt::NoModifier);
        QCoreApplication::sendEvent(control.get(), &release);
        flushInteractions();
        QVERIFY(clicks.isEmpty());
        QVERIFY(! feedback.values().pressed);
    }
    void commandDeletedInClick_data() {
        QTest::addColumn<int>("kind");
        QTest::newRow("command") << 0;
        QTest::newRow("menu-item") << 1;
    }
    void commandDeletedInClick() {
        QFETCH(int, kind);
        auto                    control = commandFor(kind);
        QPointer<CommandButton> guard(control.get());
        connect(control.get(), &CommandButton::clicked, this, [&] {
            control.reset();
        });
        control->click();
        QVERIFY(! guard);
    }
    void triggerOwner_data() {
        QTest::addColumn<int>("input");
        for (int input = 0; input < 4; ++input)
            QTest::newRow(qPrintable(QString::number(input))) << input;
    }
    void triggerOwner() {
        QFETCH(int, input);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                width: 400; height: 300
                property bool value: false
                property bool accept: false
                MD.FABMenu {
                    objectName: "menu"
                    width: 300; height: 260
                    animationsEnabled: false
                    expanded: owner.value
                    onExpandedChangeRequested: value => {
                        if (owner.accept) owner.value = value
                    }
                    MD.FABMenuItem { text: "First"; animationsEnabled: false }
                }
            }
        )",
                          QUrl("qrc:/tests/fab-trigger-owner.qml"));
        QQuickWindow             window;
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        auto* menu = object->findChild<FABMenu*>("menu");
        QVERIFY(root && menu);
        root->setParentItem(window.contentItem());
        window.resize(400, 300);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(menu->motionEnabled());
        auto* button = qobject_cast<ToggleButton*>(menu->button());
        QVERIFY(button);
        QCOMPARE((button)->metaObject()->indexOfProperty("checkable"), -1);
        QSignalSpy requests(menu, &FABMenu::expandedChangeRequested);
        const auto activate = [&] {
            const auto point =
                button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
            if (input == 0)
                button->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
            else if (input == 2) {
                button->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, point, &window).commit();
                QTest::touchEvent(&window, device).release(0, point, &window).commit();
            }
            flushInteractions();
        };
        QVERIFY(! menu->expanded() && ! button->isChecked());
        activate();
        QVERIFY(! menu->expanded() && ! button->isChecked());
        QCOMPARE(requests.size(), 1);
        object->setProperty("accept", true);
        activate();
        QVERIFY(menu->expanded() && button->isChecked());
        QCOMPARE(requests.size(), 2);
        object->setProperty("value", false);
        QVERIFY(! menu->expanded() && ! button->isChecked());
        QCOMPARE(requests.size(), 2);
        activate();
        QVERIFY(menu->expanded() && button->isChecked());
        QCOMPARE(requests.size(), 3);
        const auto escape = [&] {
            button->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(&window, Qt::Key_Escape);
        };
        escape();
        QVERIFY(! menu->expanded() && ! button->isChecked());
        QCOMPARE(requests.size(), 4);
        object->setProperty("value", true);
        object->setProperty("accept", false);
        escape();
        QVERIFY(menu->expanded() && button->isChecked());
        QCOMPARE(requests.size(), 5);
        object->setProperty("accept", true);
        escape();
        QVERIFY(! menu->expanded() && ! button->isChecked());
        QCOMPARE(requests.size(), 6);
        menu->requestExpanded(false);
        menu->setEnabled(false);
        menu->requestExpanded(true);
        QCOMPARE(requests.size(), 6);
        menu->setEnabled(true);
        QPointer<ToggleButton> alive(button);
        connect(menu, &FABMenu::expandedChangeRequested, this, [&] {
            object.reset();
        });
        button->click();
        QVERIFY(! object && ! alive);
    }
    void qmlCommand() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.CommandButtonBase {
                id: control
                property int calls: 0
                property MD.Action command: MD.Action {
                    text: "Run"
                    icon.name: "star"
                    onTriggered: control.calls++
                }
                text: command.text
                icon.actionIcon: command.icon
                enabled: command.enabled
                onClicked: command.trigger(control)
            }
        )",
                          QUrl("qrc:/tests/command-button.qml"));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* control = qobject_cast<CommandButton*>(object.get());
        auto* action  = object->property("command").value<Action*>();
        QVERIFY(control && action);
        QCOMPARE(control->text(), QString("Run"));
        QCOMPARE(control->icon()->name(), QString("star"));
        control->click();
        QCOMPARE(control->property("calls").toInt(), 1);
        action->setEnabled(false);
        control->click();
        QCOMPARE(control->property("calls").toInt(), 1);
        action->setEnabled(true);
        action->setText("Updated");
        action->icon()->setName("edit");
        control->click();
        QCOMPARE(control->property("calls").toInt(), 2);
        QCOMPARE(control->text(), QString("Updated"));
        QCOMPARE(control->icon()->name(), QString("edit"));
    }
    void replacingTrigger() {
        FABMenu       menu;
        ToggleButton  first, second;
        CommandButton command;
        QSignalSpy    requests(&menu, &FABMenu::expandedChangeRequested);
        menu.setButton(&first);
        first.click();
        QCOMPARE(requests.size(), 1);
        menu.setButton(&second);
        first.click();
        QCOMPARE(requests.size(), 1);
        second.click();
        QCOMPARE(requests.size(), 2);
        menu.setButton(nullptr);
        second.click();
        QCOMPARE(requests.size(), 2);
        menu.setButton(&command);
        command.click();
        QCOMPARE(requests.size(), 2);
        menu.setVisible(false);
        menu.requestExpanded(true);
        QCOMPARE(requests.size(), 2);
    }
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
    property MD.Action command: MD.Action { onTriggered: menu.calls++ }
    MD.FABMenuItem {
        text: "First"
        onClicked: menu.command.trigger(this)
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
        auto*                  first  = qobject_cast<FABMenuItem*>(menu->itemAt(0));
        auto*                  second = qobject_cast<FABMenuItem*>(menu->itemAt(1));
        InteractionAccumulator firstInput(this);
        firstInput.setSource(first->interactionSource());
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
        QTRY_VERIFY(firstInput.values().hovered);
        QTest::mousePress(&window,
                          Qt::LeftButton,
                          {},
                          first->mapToScene(QPointF(first->width() / 2, 28)).toPoint());
        QTRY_VERIFY(firstInput.values().pressed);
        menu->close();
        QVERIFY(! first->inputEnabled());
        QTRY_VERIFY(! firstInput.values().hovered);
        QTRY_VERIFY(! firstInput.values().pressed);
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
