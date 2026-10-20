#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include "qml_material/control/action.hpp"
#include "qml_material/control/action_group.hpp"
#include "qml_material/control/command_button.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/input/clickable.hpp"
#include "qml_material/input/interaction_source.hpp"

using namespace qml_material;

class ActionSelectionTest : public QObject {
    Q_OBJECT

private slots:
    void requestedValue() {
        Action  action;
        QObject source;
        action.setCheckable(true);
        QSignalSpy changes(&action, &Action::checkedChanged);
        QSignalSpy toggles(&action, &Action::toggled);
        QSignalSpy triggers(&action, &Action::triggered);
        connect(&action, &Action::triggered, this, [&](QObject* origin) {
            QVERIFY(action.isChecked());
            QCOMPARE(origin, &source);
        });
        action.requestChecked(true, &source);
        QCOMPARE(changes.size(), 1);
        QCOMPARE(toggles.size(), 1);
        QCOMPARE(triggers.size(), 1);
        action.requestChecked(true, &source);
        QCOMPARE(changes.size(), 1);
        QCOMPARE(toggles.size(), 1);
        QCOMPARE(triggers.size(), 2);
    }

    void rejectedRequests() {
        Action     action;
        QSignalSpy triggers(&action, &Action::triggered);
        QSignalSpy toggles(&action, &Action::toggled);
        action.requestChecked(true);
        QVERIFY(! action.isChecked());
        action.setCheckable(true);
        action.setEnabled(false);
        action.requestChecked(true);
        QVERIFY(! action.isChecked());
        action.setEnabled(true);
        ActionGroup group;
        group.addAction(&action);
        group.setEnabled(false);
        action.requestChecked(true);
        QVERIFY(! action.isChecked());
        group.setEnabled(true);
        group.setCheckedAction(&action);
        action.requestChecked(false);
        action.toggle();
        QVERIFY(action.isChecked());
        QCOMPARE(triggers.size(), 0);
        QCOMPARE(toggles.size(), 0);
        group.setCheckedAction(nullptr);
        QVERIFY(! action.isChecked());
    }

    void selectionIsAtomic() {
        Action      a, b, c;
        ActionGroup group;
        for (auto* item : { &a, &b, &c }) group.addAction(item);
        group.setCheckedAction(&a);
        int changes = 0;
        for (auto* item : { &a, &b, &c }) {
            connect(item, &Action::checkedChanged, this, [&] {
                ++changes;
                QCOMPARE(group.checkedAction(), &b);
                QVERIFY(! a.isChecked());
                QVERIFY(b.isChecked());
                QVERIFY(! c.isChecked());
            });
        }
        b.setChecked(true);
        QCOMPARE(changes, 2);
    }

    void multipleSelection() {
        Action      a, b;
        ActionGroup group;
        group.setExclusive(false);
        group.addAction(&a);
        group.addAction(&b);
        a.setChecked(true);
        b.setChecked(true);
        QVERIFY(a.isChecked() && b.isChecked());
        QVERIFY(! group.checkedAction());
        connect(&b, &Action::checkedChanged, this, [&] {
            QCOMPARE(group.checkedAction(), &a);
            QVERIFY(a.isChecked() && ! b.isChecked());
        });
        group.setExclusive(true);
        QCOMPARE(group.checkedAction(), &a);
    }

    void reentrantSelection() {
        Action      a, b, c;
        ActionGroup group;
        for (auto* item : { &a, &b, &c }) group.addAction(item);
        a.setChecked(true);
        connect(&a, &Action::checkedChanged, this, [&] {
            if (! a.isChecked()) c.setChecked(true);
        });
        b.setChecked(true);
        QCOMPARE(group.checkedAction(), &c);
        QVERIFY(! a.isChecked() && ! b.isChecked() && c.isChecked());
    }

    void memberDeletedDuringNotification() {
        Action           a;
        auto*            b = new Action;
        QPointer<Action> alive(b);
        ActionGroup      group;
        group.addAction(&a);
        group.addAction(b);
        a.setChecked(true);
        QSignalSpy membership(&group, &ActionGroup::actionsChanged);
        connect(&a, &Action::checkedChanged, this, [&] {
            delete alive.data();
        });
        b->setChecked(true);
        QVERIFY(! alive);
        QVERIFY(! group.checkedAction());
        QVERIFY(! a.isChecked());
        QCOMPARE(membership.size(), 1);
    }

    void groupDeletedDuringNotification() {
        Action                a, b;
        QPointer<ActionGroup> group(new ActionGroup);
        group->addAction(&a);
        group->addAction(&b);
        a.setChecked(true);
        QSignalSpy changes(&b, &Action::checkedChanged);
        connect(&a, &Action::checkedChanged, this, [&] {
            delete group.data();
        });
        b.setChecked(true);
        QVERIFY(! group);
        QVERIFY(! a.group() && ! b.group());
        QVERIFY(! a.isChecked() && b.isChecked());
        QCOMPARE(changes.size(), 1);
    }

    void actionDeletedDuringRequest() {
        QPointer<Action> action(new Action);
        action->setCheckable(true);
        connect(action, &Action::checkedChanged, this, [&] {
            delete action.data();
        });
        action->requestChecked(true);
        QVERIFY(! action);
    }

    void sourceDeletedDuringRequest() {
        Action action;
        action.setCheckable(true);
        QPointer<QObject> source(new QObject);
        connect(&action, &Action::checkedChanged, this, [&] {
            delete source.data();
        });
        QSignalSpy triggers(&action, &Action::triggered);
        action.requestChecked(true, source);
        QVERIFY(! source);
        QCOMPARE(triggers.size(), 1);
        QVERIFY(! triggers.at(0).at(0).value<QObject*>());
    }

    void reentrantActivation() {
        Action action;
        action.setCheckable(true);
        QSignalSpy triggers(&action, &Action::triggered);
        QSignalSpy toggles(&action, &Action::toggled);
        connect(&action, &Action::checkedChanged, this, [&] {
            action.trigger();
            action.toggle();
            action.requestChecked(false);
        });
        action.trigger();
        QVERIFY(action.isChecked());
        QCOMPARE(triggers.size(), 1);
        QCOMPARE(toggles.size(), 1);
    }

    void disableDuringRequest() {
        Action action;
        action.setCheckable(true);
        connect(&action, &Action::checkedChanged, this, [&] {
            action.setEnabled(false);
        });
        QSignalSpy triggers(&action, &Action::triggered);
        action.requestChecked(true);
        QVERIFY(action.isChecked());
        QVERIFY(! action.canTrigger());
        QCOMPARE(triggers.size(), 0);
        action.setEnabled(true);
        QVERIFY(action.canTrigger());
    }

    void membershipPreservesSelection() {
        Action      a, b;
        ActionGroup first, second;
        a.setChecked(true);
        b.setChecked(true);
        first.addAction(&a);
        second.addAction(&b);
        a.setGroup(&second);
        QVERIFY(! first.checkedAction());
        QCOMPARE(second.checkedAction(), &a);
        QVERIFY(a.isChecked() && ! b.isChecked());
        second.removeAction(&a);
        QVERIFY(a.isChecked());
        QVERIFY(! a.group());
        QVERIFY(! second.checkedAction());
        a.setChecked(false);
        QVERIFY(! a.isChecked());
    }

    void onlyOriginatingButtonClicks() {
        Action        action;
        CommandButton a, b;
        action.setCheckable(true);
        ActionCommand::qmlAttachedProperties(&a)->setAction(&action);
        ActionCommand::qmlAttachedProperties(&b)->setAction(&action);
        QSignalSpy first(&a, &CommandButton::clicked);
        QSignalSpy second(&b, &CommandButton::clicked);
        QSignalSpy triggers(&action, &Action::triggered);
        a.click();
        QVERIFY(action.isChecked());
        QCOMPARE(first.size(), 1);
        QCOMPARE(second.size(), 0);
        QCOMPARE(triggers.size(), 1);
        action.trigger();
        QCOMPARE(first.size(), 1);
        QCOMPARE(second.size(), 0);
        QCOMPARE(triggers.size(), 2);
    }

    void qmlControlledActivation() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                MD.Action {
                    id: action
                    objectName: "action"
                    checkable: true
                }
                MD.Clickable {
                    objectName: "clickable"
                    onClicked: action.requestChecked(!action.checked, this)
                }
                property bool observed: action.checked
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> root(component.create());
        QVERIFY(root);
        auto* action = root->findChild<Action*>("action");
        auto* click  = root->findChild<Clickable*>("clickable");
        QVERIFY(action && click);
        InteractionObserver observer;
        observer.setSource(click->interactionSource());
        QSignalSpy interactions(click->interactionSource(), &InteractionSource::interaction);
        click->click();
        QVERIFY(root->property("observed").toBool());
        action->setEnabled(false);
        click->click();
        QVERIFY(root->property("observed").toBool());
        QCoreApplication::processEvents();
        QCOMPARE(interactions.size(), 0);
        QVERIFY(! observer.pressed());
    }

    void actionTogglePolicy_data() {
        QTest::addColumn<bool>("checkable");
        QTest::addColumn<bool>("enabled");
        QTest::addColumn<bool>("exclusive");
        QTest::addColumn<bool>("trigger");
        QTest::addColumn<int>("toggled");
        QTest::addColumn<int>("triggered");
        QTest::addColumn<bool>("checked");
        QTest::newRow("plain-toggle") << false << true << false << false << 0 << 0 << true;
        QTest::newRow("plain-trigger") << false << true << false << true << 0 << 1 << true;
        QTest::newRow("checked-toggle") << true << true << false << false << 1 << 0 << false;
        QTest::newRow("checked-trigger") << true << true << false << true << 1 << 1 << false;
        QTest::newRow("exclusive-toggle") << true << true << true << false << 0 << 0 << true;
        QTest::newRow("exclusive-trigger") << true << true << true << true << 0 << 1 << true;
        QTest::newRow("disabled-toggle") << true << false << false << false << 0 << 0 << true;
        QTest::newRow("disabled-trigger") << true << false << false << true << 0 << 0 << true;
    }

    void actionTogglePolicy() {
        QFETCH(bool, checkable);
        QFETCH(bool, enabled);
        QFETCH(bool, exclusive);
        QFETCH(bool, trigger);
        QFETCH(int, toggled);
        QFETCH(int, triggered);
        QFETCH(bool, checked);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(
            R"(import QtQuick; import Qcm.Material as MD; MD.ActionGroup {
                property int toggles: 0
                property int triggers: 0
                property var source: QtObject {}
                property var lastSource: null
                property alias target: action
                MD.Action {
                    id: action
                    checked: true
                    onToggled: function(source) { toggles++; lastSource = source }
                    onTriggered: function(source) { triggers++; lastSource = source }
                }
                function toggleTarget() { action.toggle(source) }
                function triggerTarget() { action.trigger(source) }
            })",
            QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* target = object->property("target").value<QObject*>();
        QVERIFY(target);
        object->setProperty("exclusive", exclusive);
        target->setProperty("checkable", checkable);
        target->setProperty("enabled", enabled);
        QVERIFY(
            QMetaObject::invokeMethod(object.get(), trigger ? "triggerTarget" : "toggleTarget"));
        QCOMPARE(object->property("toggles").toInt(), toggled);
        QCOMPARE(object->property("triggers").toInt(), triggered);
        QCOMPARE(target->property("checked").toBool(), checked);
        if (toggled || triggered)
            QCOMPARE(object->property("lastSource").value<QObject*>(),
                     object->property("source").value<QObject*>());
    }
};

int run_action_selection(int argc, char** argv) {
    ActionSelectionTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "action_selection.moc"
