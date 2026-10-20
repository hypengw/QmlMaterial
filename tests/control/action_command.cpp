#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/action_command.hpp"
#include "qml_material/control/action_group.hpp"
#include "qml_material/control/button_control.hpp"
#include "qml_material/control/selection_button.hpp"
#include "qml_material/style/chip_state.hpp"
#include "qml_material/style/button_state.hpp"
#include "qml_material/style/card_state.hpp"
#include "qml_material/control/combo_box.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "../input_test.hpp"

using namespace qml_material;
namespace
{
ActionCommand* commandFor(CommandButton* button) {
    return qobject_cast<ActionCommand*>(qmlAttachedPropertiesObject<ActionCommand>(button, true));
}
CommandButton* sheetCommand(Control* bar, int index) {
    QQuickItem* item = nullptr;
    QMetaObject::invokeMethod(
        bar->contentItem(), "itemAtIndex", Q_RETURN_ARG(QQuickItem*, item), Q_ARG(int, index));
    return qobject_cast<CommandButton*>(item);
}
} // namespace

#include "qml_material/control/action.hpp"

class ActionCommandTest : public QObject {
    Q_OBJECT

private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }

    void optIn() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        for (const QByteArray type : { "Button",
                                       "IconButton",
                                       "FAB",
                                       "ExtendedFAB",
                                       "StandardIconButton",
                                       "AssistChip",
                                       "SuggestionChip",
                                       "EmbedChip",
                                       "ItemDelegate",
                                       "ListItem",
                                       "MenuItem",
                                       "SearchBar",
                                       "Card" }) {
            QQmlComponent component(&engine);
            component.setData("import Qcm.Material as MD\nMD." + type + " {}", QUrl());
            std::unique_ptr<QObject> object(component.create());
            QVERIFY2(object, qPrintable(component.errorString()));
            auto* button = qobject_cast<CommandButton*>(object.get());
            QVERIFY(button);
            QVERIFY2(! qmlAttachedPropertiesObject<ActionCommand>(button, false), type.constData());
            QSignalSpy clicks(button, &CommandButton::clicked);
            button->click();
            QCOMPARE(clicks.count(), 1);
            QVERIFY2(! qmlAttachedPropertiesObject<ActionCommand>(button, false), type.constData());
        }
    }

    void projection() {
        CommandButton button;
        Action        first, second;
        first.setText("First");
        first.icon()->setName("add");
        first.setCheckable(true);
        auto* command = commandFor(&button);
        QCOMPARE(commandFor(&button), command);
        command->setAction(&first);
        QCOMPARE(command->action(), &first);
        QCOMPARE(command->text(), first.text());
        QCOMPARE(command->icon(), first.icon());
        QVERIFY(command->isEnabled());
        QCOMPARE(button.text(), QString());
        QVERIFY(button.icon()->isEmpty());
        QSignalSpy texts(command, &ActionCommand::textChanged);
        QSignalSpy enabled(command, &ActionCommand::enabledChanged);
        first.setText("Changed");
        QCOMPARE(texts.count(), 1);
        QCOMPARE(command->text(), QString("Changed"));
        first.setEnabled(false);
        QVERIFY(! command->isEnabled());
        QVERIFY(button.isEnabled());
        QCOMPARE(enabled.count(), 1);
        QSignalSpy triggered(&first, &Action::triggered);
        button.click();
        QVERIFY(triggered.isEmpty());
        first.setEnabled(true);
        button.click();
        QCOMPARE(triggered.count(), 1);
        QCOMPARE(triggered.first().first().value<QObject*>(), &button);
        QVERIFY(first.isChecked());
        QCOMPARE(button.metaObject()->indexOfProperty("checked"), -1);
        command->setAction(&second);
        texts.clear();
        first.setText("Detached");
        QVERIFY(texts.isEmpty());
        button.click();
        QCOMPARE(triggered.count(), 1);
        command->setAction(nullptr);
        QVERIFY(! command->action());
        QVERIFY(! command->icon());
        QVERIFY(command->isEnabled());
    }

    void retargetInClickObserver() {
        CommandButton button;
        Action        first, second;
        connect(&button, &CommandButton::clicked, this, [&] {
            commandFor(&button)->setAction(&second);
        });
        auto* command = commandFor(&button);
        command->setAction(&first);
        QSignalSpy previous(&first, &Action::triggered);
        QSignalSpy next(&second, &Action::triggered);
        button.click();
        QVERIFY(previous.isEmpty() && next.isEmpty());
        button.click();
        QCOMPARE(next.count(), 1);
        QVERIFY(previous.isEmpty());
    }

    void reentry() {
        CommandButton button;
        Action        first, second, third;
        third.setText("Third");
        auto* command = commandFor(&button);
        command->setAction(&first);
        connect(command, &ActionCommand::actionChanged, this, [&] {
            if (command->action() == &second) command->setAction(&third);
        });
        QSignalSpy texts(command, &ActionCommand::textChanged);
        QSignalSpy enabled(command, &ActionCommand::enabledChanged);
        command->setAction(&second);
        QCOMPARE(command->action(), &third);
        QCOMPARE(command->text(), QString("Third"));
        QCOMPARE(texts.count(), 1);
        QCOMPARE(enabled.count(), 1);
        auto* doomed = new Action;
        command->setAction(doomed);
        texts.clear();
        enabled.clear();
        delete doomed;
        QVERIFY(! command->action());
        QCOMPARE(texts.count(), 1);
        QCOMPARE(enabled.count(), 1);
    }

    void deletedInNotification_data() {
        QTest::addColumn<int>("notification");
        QTest::addColumn<bool>("wholeButton");
        for (int notification = 0; notification < 3; ++notification)
            for (bool wholeButton : { false, true })
                QTest::newRow(qPrintable(QString("%1-%2").arg(notification).arg(wholeButton)))
                    << notification << wholeButton;
    }

    void deletedInNotification() {
        QFETCH(int, notification);
        QFETCH(bool, wholeButton);
        QPointer<CommandButton> button  = new CommandButton;
        QPointer<ActionCommand> command = commandFor(button);
        Action                  first, second;
        command->setAction(&first);
        const auto destroy = [&] {
            if (wholeButton)
                delete button.data();
            else
                delete command.data();
        };
        if (notification == 0) {
            connect(command, &ActionCommand::actionChanged, this, destroy);
            command->setAction(&second);
        } else if (notification == 1) {
            connect(command, &ActionCommand::textChanged, this, destroy);
            command->setAction(&second);
        } else {
            connect(command, &ActionCommand::enabledChanged, this, destroy);
            first.setEnabled(false);
        }
        QVERIFY(! command);
        if (wholeButton) QVERIFY(! button);
        delete button.data();
    }

    void styledCommands_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<int>("input");
        for (const QByteArray type : { "Button",
                                       "IconButton",
                                       "FAB",
                                       "ExtendedFAB",
                                       "StandardIconButton",
                                       "AssistChip",
                                       "SuggestionChip",
                                       "EmbedChip",
                                       "ItemDelegate",
                                       "ListItem",
                                       "MenuItem",
                                       "SearchBar",
                                       "Card" })
            for (int input = 0; input < 4; ++input)
                QTest::newRow(qPrintable(QString::fromLatin1(type) + QString::number(input)))
                    << type << input;
    }

    void styledCommands() {
        QFETCH(QByteArray, type);
        QFETCH(int, input);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport Qcm.Material as MD\nMD." + type + R"( {
            width: 160; height: 56
            MD.ActionCommand.action: MD.Action { text: "Create"; icon.name: "add" }
            text: MD.ActionCommand.text
            icon.actionIcon: MD.ActionCommand.icon
            enabled: MD.ActionCommand.enabled
            function useLocalText() { text = "Local" }
        })",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<CommandButton*>(object.get());
        QVERIFY(button);
        QCOMPARE((button)->metaObject()->indexOfProperty("checkable"), -1);
        for (const char* name : { "action", "checked", "checkable", "pressed", "pressX", "pressY" })
            QCOMPARE(button->metaObject()->indexOfProperty(name), -1);
        if (auto* combined = qobject_cast<CombinedCommandButton*>(button)) {
            QVERIFY(! combined->longClickEnabled());
            QVERIFY(! combined->doubleClickEnabled());
        }
        auto* command = commandFor(button);
        auto* action  = command->action();
        QCOMPARE(button->text(), action->text());
        QCOMPARE(button->icon()->name(), action->icon()->name());
        action->setText("Compose");
        QCOMPARE(button->text(), action->text());
        QVERIFY(QMetaObject::invokeMethod(button, "useLocalText"));
        action->setText("New");
        QCOMPARE(button->text(), QString("Local"));
        QQuickWindow window;
        window.resize(240, 120);
        button->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy triggered(action, &Action::triggered);
        const auto activate = [&] {
            if (input == 0)
                button->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
            else if (input == 2) {
                button->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, { 20, 20 }, &window).commit();
                QTest::touchEvent(&window, device).release(0, { 20, 20 }, &window).commit();
            }
            flushInteractions();
        };
        activate();
        QCOMPARE(triggered.count(), 1);
        QCOMPARE(triggered.first().first().value<QObject*>(), button);
        action->setEnabled(false);
        QVERIFY(! button->isEnabled());
        activate();
        QCOMPARE(triggered.count(), 1);
        action->setEnabled(true);
        activate();
        QCOMPARE(triggered.count(), 2);
    }

    void commandCancellation_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<int>("change");
        for (const QByteArray type :
             { "ItemDelegate", "ListItem", "MenuItem", "SearchBar", "Card" })
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString::fromLatin1(type) + QString::number(change)))
                    << type << change;
    }

    void commandCancellation() {
        QFETCH(QByteArray, type);
        QFETCH(int, change);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD\nMD." + type + " { width: 160; height: 56 }",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<CommandButton*>(object.get());
        QVERIFY(button);
        InteractionAccumulator feedback(this);
        feedback.setSource(button->interactionSource());
        QSignalSpy clicks(button, &CommandButton::clicked);
        setButtonPressed(button, true);
        QVERIFY(feedback.values().pressed);
        InteractionSource replacement;
        if (change == 0)
            button->setEnabled(false);
        else if (change == 1)
            button->setVisible(false);
        else
            button->setInteractionSource(&replacement);
        QMouseEvent release(QEvent::MouseButtonRelease,
                            { 20, 20 },
                            { 20, 20 },
                            Qt::LeftButton,
                            Qt::NoButton,
                            Qt::NoModifier);
        QCoreApplication::sendEvent(button, &release);
        flushInteractions();
        QVERIFY(clicks.isEmpty() && ! feedback.values().pressed);
    }

    void cardSurfaces_data() {
        QTest::addColumn<int>("type");
        QTest::newRow("elevated") << int(Enum::CardType::CardElevated);
        QTest::newRow("filled") << int(Enum::CardType::CardFilled);
        QTest::newRow("outlined") << int(Enum::CardType::CardOutlined);
    }

    void cardSurfaces() {
        QFETCH(int, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD\nMD.Card {}", QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "type", type } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* card = qobject_cast<CommandButton*>(object.get());
        QVERIFY(card && card->background());
        auto* state = card->property("mdState").value<CardState*>();
        QVERIFY(state);
        QCOMPARE(state->type(), type);
        QCOMPARE(state->item(), card);
        QCOMPARE(state->interactionSource(), card->interactionSource());
        QCOMPARE(card->implicitWidth(), 64.0);
        QCOMPARE(card->implicitHeight(), 64.0);
        const bool elevated = type == int(Enum::CardType::CardElevated);
        QCOMPARE(card->background()->property("elevationVisible").toBool(), elevated);
        QVERIFY(card->setProperty("flat", true));
        QVERIFY(! card->background()->property("elevationVisible").toBool());
        QVERIFY(card->setProperty("flat", false));
        QCOMPARE(card->background()->property("elevationVisible").toBool(), elevated);
        card->setEnabled(false);
        QCOMPARE(state->contentOpacity(), state->stateTokens().disabled_content);
        QCOMPARE(state->backgroundOpacity(), state->stateTokens().disabled_container);
        card->setEnabled(true);
        QCOMPARE(state->contentOpacity(), 1.0);
        QCOMPARE(state->backgroundOpacity(), 1.0);
    }

    void sheetCommands_data() {
        QTest::addColumn<int>("input");
        for (int input = 0; input < 4; ++input)
            QTest::newRow(qPrintable(QString::number(input))) << input;
    }

    void sheetCommands() {
        QFETCH(int, input);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.SheetActionBar {
                width: 320
                property MD.Action first: MD.Action {
                    text: "First"; icon.name: "share"; checkable: true; checked: true
                }
                property MD.Action second: MD.Action {
                    text: "Second"; icon.name: "archive"; checkable: true
                }
                actions: [first, second, null]
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<Control*>(object.get());
        QVERIFY(bar);
        QQuickWindow window;
        window.resize(400, 160);
        bar->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        CommandButton *first = nullptr, *second = nullptr, *empty = nullptr;
        QTRY_VERIFY((first = sheetCommand(bar, 0)));
        QTRY_VERIFY((second = sheetCommand(bar, 1)));
        QTRY_VERIFY((empty = sheetCommand(bar, 2)));
        QCOMPARE(bar->implicitHeight(), 96.0);
        QCOMPARE(first->height(), 80.0);
        QVERIFY(first->width() >= 72.0);
        QTRY_COMPARE(second->x(), first->x() + first->width() + 8);
        auto* firstAction  = commandFor(first)->action();
        auto* secondAction = commandFor(second)->action();
        QVERIFY(firstAction && secondAction);
        ActionGroup group;
        group.addAction(firstAction);
        group.addAction(secondAction);
        QCOMPARE(group.checkedAction(), firstAction);
        auto* firstState  = first->property("mdState").value<ButtonState*>();
        auto* secondState = second->property("mdState").value<ButtonState*>();
        QVERIFY(firstState && secondState);
        QVERIFY(firstState->isSelected() && ! secondState->isSelected());
        for (auto* button : { first, second, empty }) {
            QCOMPARE((button)->metaObject()->indexOfProperty("checkable"), -1);
            for (const char* name :
                 { "action", "checked", "checkable", "pressed", "pressX", "pressY" })
                QCOMPARE(button->metaObject()->indexOfProperty(name), -1);
            QVERIFY(! button->metaObject()
                          ->property(button->metaObject()->indexOfProperty("selected"))
                          .isWritable());
        }
        QVERIFY(! commandFor(empty)->action() && ! empty->isEnabled());
        QSignalSpy emptyClicks(empty, &CommandButton::clicked);
        empty->click();
        QVERIFY(emptyClicks.isEmpty());
        QSignalSpy triggers(secondAction, &Action::triggered),
            toggles(secondAction, &Action::toggled);
        const auto activate = [&] {
            const auto point =
                second->mapToScene({ second->width() / 2, second->height() / 2 }).toPoint();
            if (input == 0)
                second->click();
            else if (input == 1)
                QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
            else if (input == 2) {
                second->forceActiveFocus(Qt::TabFocusReason);
                QTest::keyClick(&window, Qt::Key_Space);
            } else {
                static auto* device = QTest::createTouchDevice();
                QTest::touchEvent(&window, device).press(0, point, &window).commit();
                QTest::touchEvent(&window, device).release(0, point, &window).commit();
            }
            flushInteractions();
        };
        activate();
        QCOMPARE(triggers.size(), 1);
        QCOMPARE(triggers.first().first().value<QObject*>(), second);
        QCOMPARE(group.checkedAction(), secondAction);
        QVERIFY(! firstState->isSelected() && secondState->isSelected());
        QVERIFY(! first->icon()->fill() && second->icon()->fill());
        activate();
        QCOMPARE(triggers.size(), 2);
        QCOMPARE(toggles.size(), 1);
        QVERIFY(secondState->isSelected());
        group.setCheckedAction(firstAction);
        QVERIFY(firstState->isSelected() && ! secondState->isSelected());
        secondAction->setText("Changed");
        secondAction->icon()->setName("add");
        QCOMPARE(second->text(), QString("Changed"));
        QCOMPARE(second->icon()->name(), QString("add"));
        secondAction->setCheckable(false);
        QVERIFY(! secondState->isToggleable());
        group.setEnabled(false);
        QVERIFY(! first->isEnabled() && ! second->isEnabled());
        activate();
        QCOMPARE(triggers.size(), 2);
        group.setEnabled(true);
        activate();
        QCOMPARE(triggers.size(), 3);
        QCOMPARE(group.checkedAction(), firstAction);
        QVERIFY(! secondState->isSelected());
        object.reset();
        flushInteractions();
    }

    void sheetRetarget_data() { retarget_data(); }

    void sheetRetarget() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.SheetActionBar {
                width: 320
                property MD.Action first: MD.Action { text: "First"; icon.name: "share" }
                property MD.Action second: MD.Action { text: "Second"; icon.name: "add" }
                property bool useSecond: false
                actions: [useSecond ? second : first]
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<Control*>(object.get());
        QVERIFY(bar);
        QQuickWindow window;
        window.resize(400, 160);
        bar->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QPointer<CommandButton> button;
        QTRY_VERIFY((button = sheetCommand(bar, 0)));
        auto* first  = bar->property("first").value<Action*>();
        auto* second = bar->property("second").value<Action*>();
        QVERIFY(first && second);
        QSignalSpy previous(first, &Action::triggered), next(second, &Action::triggered);
        const auto point =
            button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
        if (keyboard) {
            button->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(&window, Qt::Key_Space);
        } else {
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        }
        if (change == 0)
            bar->setProperty("useSecond", true);
        else if (change == 1)
            delete first;
        else
            first->setEnabled(false);
        if (keyboard)
            QTest::keyRelease(&window, Qt::Key_Space);
        else
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        flushInteractions();
        QVERIFY(previous.isEmpty() && next.isEmpty());
        QVERIFY(button);
        if (change != 0) QVERIFY(! button->isEnabled());
        bar->setProperty("useSecond", true);
        QCOMPARE(commandFor(button)->action(), second);
        QCOMPARE(button->text(), QString("Second"));
        QCOMPARE(button->icon()->name(), QString("add"));
        button->click();
        QCOMPARE(next.size(), 1);
        object.reset();
        flushInteractions();
    }

    void sheetDeletedInTrigger_data() {
        QTest::addColumn<bool>("selectionNotification");
        QTest::newRow("trigger") << false;
        QTest::newRow("selection") << true;
    }

    void sheetDeletedInTrigger() {
        QFETCH(bool, selectionNotification);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.SheetActionBar {
                width: 320
                actions: [MD.Action { text: "Remove"; checkable: true }]
            }
        )",
                          QUrl());
        QPointer<Control> bar = qobject_cast<Control*>(component.create());
        QVERIFY2(bar, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(400, 160);
        bar->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QPointer<CommandButton> button;
        QTRY_VERIFY((button = sheetCommand(bar, 0)));
        auto* action = commandFor(button)->action();
        QVERIFY(action);
        int        calls   = 0;
        const auto destroy = [&] {
            ++calls;
            delete bar.data();
        };
        if (selectionNotification)
            connect(action, &Action::checkedChanged, this, destroy);
        else
            connect(action, &Action::triggered, this, destroy);
        button->click();
        QCOMPARE(calls, 1);
        QVERIFY(! bar && ! button);
        flushInteractions();
    }

    void delegateValues_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<QByteArray>("setup");
        QTest::newRow("table")
            << QByteArray("TableViewDelegate")
            << QByteArray("current: false; editing: false; model: ({display: \"One\"})");
        QTest::newRow("horizontal-header")
            << QByteArray("HorizontalHeaderViewDelegate") << QByteArray();
        QTest::newRow("vertical-header")
            << QByteArray("VerticalHeaderViewDelegate") << QByteArray();
    }

    void delegateValues() {
        QFETCH(QByteArray, type);
        QFETCH(QByteArray, setup);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: owner
                property bool value: false
                MD.)" + type + R"( {
                    objectName: "control"
                    row: 0; column: 0
                    selected: owner.value
        )" + setup + R"(
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = object->findChild<ItemDelegate*>("control");
        QVERIFY(button);
        QCOMPARE((button)->metaObject()->indexOfProperty("checkable"), -1);
        QSignalSpy clicks(button, &CommandButton::clicked);
        button->click();
        QVERIFY(! button->property("selected").toBool() && ! button->isHighlighted());
        object->setProperty("value", true);
        QVERIFY(button->property("selected").toBool() && button->isHighlighted());
        button->click();
        QVERIFY(button->property("selected").toBool() && button->isHighlighted());
        object->setProperty("value", false);
        QVERIFY(! button->property("selected").toBool() && ! button->isHighlighted());
        QCOMPARE(clicks.size(), 2);
    }

    void comboBoxCommands_data() {
        QTest::addColumn<QByteArray>("type");
        for (const QByteArray type : { "ItemDelegate", "ListItem", "MenuItem" })
            QTest::newRow(type.constData()) << type;
    }

    void comboBoxCommands() {
        QFETCH(QByteArray, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.ComboBox {
                id: combo
                width: 240
                model: ["One", "Two", "Three"]
                delegate: MD.)" +
                              type + R"( {
                    required property var model
                    required property int index
                    objectName: "choice" + index
                    text: model.modelData
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* combo = qobject_cast<ComboBox*>(object.get());
        QVERIFY(combo);
        QQuickWindow window;
        window.resize(400, 300);
        combo->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        combo->openPopup();
        QTRY_VERIFY(combo->popupVisible());
        const auto find = [](QQuickItem* item, const auto& find) -> CommandButton* {
            if (item->objectName() == "choice1") return qobject_cast<CommandButton*>(item);
            for (auto* child : item->childItems())
                if (auto* found = find(child, find)) return found;
            return nullptr;
        };
        CommandButton* button = nullptr;
        QTRY_VERIFY((button = find(window.contentItem(), find)));
        QSignalSpy accepted(combo, &ComboBox::selectionChangeRequested);
        connect(
            combo, &ComboBox::selectionChangeRequested, combo, [combo](int index, const QVariant&) {
                combo->setCurrentIndex(index);
            });
        button->click();
        QCOMPARE(combo->currentIndex(), 1);
        QCOMPARE(combo->currentText(), QString("Two"));
        QCOMPARE(accepted.size(), 1);
        QTRY_VERIFY(! combo->popupVisible());
        combo->openPopup();
        QTRY_VERIFY(combo->popupVisible());
        combo->highlightIndex(0);
        QCOMPARE(combo->highlightedIndex(), 0);
        auto producer = button->interactionSource()->emitter(button);
        producer->begin(Interaction::Hover);
        flushInteractions();
        QCOMPARE(combo->highlightedIndex(), 1);
        producer->cancel();
    }

    void inputChipCommands_data() {
        QTest::addColumn<int>("input");
        for (int input = 0; input < 4; ++input)
            QTest::newRow(qPrintable(QString::number(input))) << input;
    }

    void inputChipCommands() {
        QFETCH(int, input);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.InputChip {
                text: "Chip"
                selected: true
                trailingAction: MD.Action {
                    text: "Remove"
                    icon.name: "close"
                    checkable: true
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* chip = qobject_cast<SelectionButton*>(object.get());
        QVERIFY(chip);
        auto* button = chip->findChild<CommandButton*>();
        QVERIFY(button);
        auto* action = commandFor(button)->action();
        QVERIFY(action);
        QCOMPARE(chip->text(), QString("Chip"));
        QCOMPARE(chip->icon()->name(), QString("close"));
        QQuickWindow window;
        window.resize(300, 120);
        chip->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QTRY_VERIFY(window.isActive());
        QTRY_VERIFY(button->width() > 0 && button->height() > 0);
        QSignalSpy requests(chip, &SelectionButton::selectionRequested);
        QSignalSpy triggers(action, &Action::triggered);
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
        activate();
        QCOMPARE(triggers.size(), 1);
        QCOMPARE(triggers.first().first().value<QObject*>(), button);
        QVERIFY(requests.isEmpty() && chip->isSelected() && action->isChecked());
        chip->click();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(triggers.size(), 1);
        action->setEnabled(false);
        QVERIFY(chip->isEnabled() && ! button->isEnabled());
        activate();
        QCOMPARE(triggers.size(), 1);
        // Qt omits disabled children from hit testing, unlike Compose clickable.
        const int parentRequests = (input == 1 || input == 3) ? 1 : 0;
        QCOMPARE(requests.size(), 1 + parentRequests);
        chip->click();
        QCOMPARE(requests.size(), 2 + parentRequests);
        auto* state = chip->findChild<InputChipState*>();
        QVERIFY(state && state->isSelected());
        chip->setSelected(false);
        QVERIFY(! state->isSelected() && action->isChecked());
        action->setEnabled(true);
        activate();
        QCOMPARE(triggers.size(), 2);
        QCOMPARE(requests.size(), 2 + parentRequests);
        QVERIFY(! chip->isSelected() && ! action->isChecked());
    }

    void inputChipRetarget_data() { retarget_data(); }

    void inputChipRetarget() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(
            "import Qcm.Material as MD\nMD.InputChip { text: \"Chip\"; selected: true }", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* chip = qobject_cast<SelectionButton*>(object.get());
        QVERIFY(chip);
        auto first = std::make_unique<Action>();
        first->icon()->setName("close");
        Action second;
        second.icon()->setName("add");
        QVERIFY(chip->setProperty("trailingAction", QVariant::fromValue(first.get())));
        auto* button = chip->findChild<CommandButton*>();
        QVERIFY(button);
        QQuickWindow window;
        window.resize(300, 120);
        chip->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.requestActivate();
        QTRY_VERIFY(window.isActive());
        QSignalSpy previous(first.get(), &Action::triggered), next(&second, &Action::triggered);
        QSignalSpy requests(chip, &SelectionButton::selectionRequested);
        const auto point =
            button->mapToScene({ button->width() / 2, button->height() / 2 }).toPoint();
        if (keyboard) {
            button->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(&window, Qt::Key_Space);
        } else {
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        }
        if (change == 0)
            chip->setProperty("trailingAction", QVariant::fromValue(&second));
        else if (change == 1)
            first.reset();
        else
            first->setEnabled(false);
        if (keyboard)
            QTest::keyRelease(&window, Qt::Key_Space);
        else
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        flushInteractions();
        QVERIFY(previous.isEmpty() && next.isEmpty() && requests.isEmpty());
        QVERIFY(chip->isSelected());
        chip->setProperty("trailingAction", QVariant::fromValue(&second));
        QCOMPARE(chip->icon()->name(), QString("add"));
        button->click();
        QCOMPARE(next.size(), 1);
    }

    void inputChipDeletedInTrigger() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.InputChip {
                trailingAction: MD.Action { icon.name: "close" }
            }
        )",
                          QUrl());
        QPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = object->findChild<CommandButton*>();
        QVERIFY(button);
        auto*      action = commandFor(button)->action();
        QSignalSpy triggered(action, &Action::triggered);
        connect(action, &Action::triggered, this, [&] {
            delete object.data();
        });
        button->click();
        QCOMPARE(triggered.size(), 1);
        QVERIFY(! object);
    }

    void retarget_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::addColumn<int>("change");
        for (bool keyboard : { false, true })
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(keyboard).arg(change)))
                    << keyboard << change;
    }

    void retarget() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        QQuickWindow window;
        window.resize(200, 100);
        CommandButton button(window.contentItem());
        button.setSize({ 100, 50 });
        auto   first = std::make_unique<Action>();
        Action second;
        auto*  command = commandFor(&button);
        command->setAction(first.get());
        QSignalSpy triggers(first.get(), &Action::triggered);
        QSignalSpy nextTriggers(&second, &Action::triggered);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (keyboard) {
            button.forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(&window, Qt::Key_Space);
        } else {
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        }
        if (change == 0)
            command->setAction(&second);
        else if (change == 1)
            first.reset();
        else
            first->setEnabled(false);
        if (keyboard)
            QTest::keyRelease(&window, Qt::Key_Space);
        else
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QVERIFY(triggers.isEmpty());
        QVERIFY(nextTriggers.isEmpty());
        command->setAction(&second);
        button.click();
        QCOMPARE(nextTriggers.count(), 1);
    }

    void headerActionRetarget() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.NavigationRail {
                width: 220; height: 400
                MD.MProp.size: MD.ThemeSize { width: 1280; duration: 0 }
                autoExpand: false
                headerAction: MD.Action { objectName: "headerAction" }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto*        root = qobject_cast<QQuickItem*>(object.get());
        QQuickWindow window;
        window.resize(1280, 500);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* action = root->findChild<Action*>("headerAction");
        QVERIFY(action);
        CommandButton* button = nullptr;
        for (auto* candidate : root->findChildren<CommandButton*>())
            if (commandFor(candidate)->action() == action) button = candidate;
        QVERIFY(button);
        connect(action, &Action::triggered, this, [&] {
            root->setProperty("headerAction", QVariant::fromValue<Action*>(nullptr));
        });
        button->click();
        QVERIFY(! root->property("expanded").toBool());
        QCOMPARE(commandFor(button)->action(),
                 root->property("defaultHeaderAction").value<Action*>());
        button->click();
        QVERIFY(root->property("expanded").toBool());
    }

    void deletedInTrigger_data() {
        QTest::addColumn<QByteArray>("type");
        for (const QByteArray type : { "FAB",
                                       "ExtendedFAB",
                                       "StandardIconButton",
                                       "NavigationRail",
                                       "AssistChip",
                                       "SuggestionChip",
                                       "EmbedChip",
                                       "ItemDelegate",
                                       "ListItem",
                                       "MenuItem",
                                       "SearchBar",
                                       "Card" })
            QTest::newRow(type.constData()) << type;
    }

    void deletedInTrigger() {
        QFETCH(QByteArray, type);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        const auto    content = type == "NavigationRail" ? QByteArray(R"(
            width: 220; height: 400
            MD.MProp.size: MD.ThemeSize { width: 1280; duration: 0 }
            headerAction: MD.Action { objectName: "headerAction" }
        )")
                                                         : QByteArray(R"(
            MD.ActionCommand.action: MD.Action { objectName: "headerAction" }
        )");
        component.setData("import Qcm.Material as MD\nMD." + type + " {" + content + "}", QUrl());
        QPointer<QQuickItem> root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(1280, 500);
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* action = root->findChild<Action*>("headerAction");
        QVERIFY(action);
        CommandButton* button = qobject_cast<CommandButton*>(root);
        if (! button) {
            const auto candidates = root->findChildren<CommandButton*>();
            for (auto* candidate : candidates)
                if (commandFor(candidate)->action() == action) button = candidate;
        }
        QVERIFY(button);
        int count = 0;
        connect(action, &Action::triggered, this, [&](QObject* source) {
            QCOMPARE(source, button);
            ++count;
            delete root.data();
        });
        button->click();
        QCOMPARE(count, 1);
        QVERIFY(! root);
    }

    void actionSynchronization() {
        QProperty<bool> checkable(true);
        Action          action;
        CommandButton   button, peer;
        action.bindableCheckable().setBinding([&] {
            return checkable.value();
        });
        auto* first  = ActionCommand::qmlAttachedProperties(&button);
        auto* second = ActionCommand::qmlAttachedProperties(&peer);
        first->setAction(&action);
        second->setAction(&action);
        button.click();
        QVERIFY(action.isChecked());
        QCOMPARE(button.metaObject()->indexOfProperty("checked"), -1);
        QCOMPARE(peer.metaObject()->indexOfProperty("checkable"), -1);
        peer.click();
        QVERIFY(! action.isChecked());
        checkable = false;
        QVERIFY(! action.isCheckable());
        checkable = true;
        QVERIFY(action.isCheckable());
        QVERIFY(action.bindableCheckable().hasBinding());
    }

    void actionRetargetsFeedbackSource() {
        CombinedCommandButton button;
        Action                action;
        InteractionSource     source;
        auto*                 command = ActionCommand::qmlAttachedProperties(&button);
        command->setAction(&action);
        QSignalSpy clicks(&button, &CommandButton::clicked);
        connect(&action, &Action::triggered, this, [&] {
            const QSignalBlocker blocked(&button);
            button.setInteractionSource(&source);
        });
        button.click();
        QCOMPARE(clicks.size(), 1);
        QCOMPARE(button.interactionSource(), &source);
    }
};

int run_action_command(int argc, char** argv) {
    ActionCommandTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "action_command.moc"
