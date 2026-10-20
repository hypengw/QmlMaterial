#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/input/clickable.hpp"
#include "qml_material/input/selectable.hpp"
#include "qml_material/input/toggleable.hpp"

using namespace qml_material;
namespace
{
void key(QQuickItem& item, QEvent::Type type, int code) {
    QKeyEvent event(type, code, Qt::NoModifier);
    QCoreApplication::sendEvent(&item, &event);
}
QList<Interaction> presses(const QSignalSpy& spy) {
    QList<Interaction> result;
    for (const auto& row : spy) {
        const auto event = row.front().value<Interaction>();
        if (event.kind == Interaction::Press) result.append(event);
    }
    return result;
}
} // namespace

class ControlledInputTest : public QObject {
    Q_OBJECT

private slots:
    void independentSemantics() {
        Clickable  command;
        Toggleable toggle;
        Selectable selection;
        for (auto* item : QList<InteractiveItem*> { &command, &toggle, &selection }) {
            for (const char* name : { "pressed",
                                      "hovered",
                                      "pressX",
                                      "pressY",
                                      "checkable",
                                      "autoExclusive",
                                      "action",
                                      "inputInteractions" })
                QCOMPARE(item->metaObject()->indexOfProperty(name), -1);
            QVERIFY(item->interactionSource());
        }
        QCOMPARE(command.metaObject()->indexOfProperty("checked"), -1);
        QCOMPARE(toggle.metaObject()->indexOfProperty("selected"), -1);
        QCOMPARE(selection.metaObject()->indexOfProperty("checked"), -1);
        QCOMPARE(toggle.metaObject()->indexOfSignal("clicked()"), -1);
        QCOMPARE(selection.metaObject()->indexOfSignal("clicked()"), -1);
    }

    void toggleRequestsLatestValue() {
        Toggleable      item;
        QProperty<bool> owner(false);
        item.bindableChecked().setBinding([&] {
            return owner.value();
        });
        QSignalSpy requests(&item, &Toggleable::checkedChangeRequested);
        QSignalSpy changed(&item, &Toggleable::checkedChanged);
        item.click();
        item.click();
        QCOMPARE(requests.size(), 2);
        QVERIFY(requests[0][0].toBool());
        QVERIFY(requests[1][0].toBool());
        QVERIFY(! item.isChecked());
        QVERIFY(changed.isEmpty());
        QVERIFY(item.bindableChecked().hasBinding());
        connect(&item, &Toggleable::checkedChangeRequested, &item, [&](bool value) {
            owner = value;
        });
        item.click();
        QVERIFY(item.isChecked());
        item.click();
        QVERIFY(! item.isChecked());
        QCOMPARE(changed.size(), 2);
        QVERIFY(item.bindableChecked().hasBinding());
    }

    void selectionIsNotToggle() {
        Selectable     first, second;
        QProperty<int> owner(0);
        first.bindableSelected().setBinding([&] {
            return owner.value() == 0;
        });
        second.bindableSelected().setBinding([&] {
            return owner.value() == 1;
        });
        QSignalSpy requests(&first, &Selectable::selectionRequested);
        QSignalSpy changed(&first, &Selectable::selectedChanged);
        first.click();
        QCOMPARE(requests.size(), 1);
        QVERIFY(first.isSelected());
        QVERIFY(changed.isEmpty());
        second.click();
        QVERIFY(! second.isSelected());
        connect(&second, &Selectable::selectionRequested, &second, [&] {
            owner = 1;
        });
        second.click();
        QVERIFY(! first.isSelected());
        QVERIFY(second.isSelected());
        QVERIFY(first.bindableSelected().hasBinding());
        QVERIFY(second.bindableSelected().hasBinding());
    }

    void stateChangeDuringPress() {
        Toggleable item;
        QSignalSpy requests(&item, &Toggleable::checkedChangeRequested);
        QSignalSpy input(item.interactionSource(), &InteractionSource::interaction);
        key(item, QEvent::KeyPress, Qt::Key_Space);
        item.setChecked(true);
        key(item, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(requests.size(), 1);
        QVERIFY(! requests[0][0].toBool());
        QVERIFY(item.isChecked());
        QTRY_COMPARE(presses(input).size(), 2);
        QCOMPARE(presses(input)[0].phase, Interaction::Begin);
        QCOMPARE(presses(input)[1].phase, Interaction::End);
        QCOMPARE(presses(input)[0].id, presses(input)[1].id);
    }

    void semanticActivationHasNoPress() {
        Toggleable        toggle;
        Selectable        selection;
        InteractionSource source;
        toggle.setInteractionSource(&source);
        selection.setInteractionSource(&source);
        QSignalSpy input(&source, &InteractionSource::interaction);
        toggle.click();
        selection.click();
        QCoreApplication::sendPostedEvents(&source);
        QVERIFY(input.isEmpty());
    }

    void unavailableDoesNotRequest() {
        QQuickWindow window;
        Toggleable   toggle(window.contentItem());
        Selectable   selection(window.contentItem());
        QSignalSpy   toggles(&toggle, &Toggleable::checkedChangeRequested);
        QSignalSpy   selections(&selection, &Selectable::selectionRequested);
        for (auto* item : QList<InteractiveItem*> { &toggle, &selection }) {
            item->setEnabled(false);
            item->click();
            item->setEnabled(true);
            item->setVisible(false);
            item->click();
        }
        QVERIFY(toggles.isEmpty());
        QVERIFY(selections.isEmpty());
    }

    void cancellation_data() {
        QTest::addColumn<int>("reason");
        QTest::newRow("disabled") << 0;
        QTest::newRow("hidden") << 1;
        QTest::newRow("source-replaced") << 2;
        QTest::newRow("source-destroyed") << 3;
        QTest::newRow("item-destroyed") << 4;
    }

    void cancellation() {
        QFETCH(int, reason);
        QQuickWindow      window;
        auto              item   = std::make_unique<Toggleable>(window.contentItem());
        auto              source = std::make_unique<InteractionSource>();
        InteractionSource replacement;
        item->setInteractionSource(source.get());
        QSignalSpy input(source.get(), &InteractionSource::interaction);
        int        requests = 0;
        connect(item.get(), &Toggleable::checkedChangeRequested, this, [&](bool) {
            ++requests;
        });
        key(*item, QEvent::KeyPress, Qt::Key_Space);
        QTRY_COMPARE(presses(input).size(), 1);
        switch (reason) {
        case 0: item->setEnabled(false); break;
        case 1: item->setVisible(false); break;
        case 2: item->setInteractionSource(&replacement); break;
        case 3: source.reset(); break;
        case 4: item.reset(); break;
        }
        if (item) key(*item, QEvent::KeyRelease, Qt::Key_Space);
        QCOMPARE(requests, 0);
        if (source) {
            QTRY_COMPARE(presses(input).size(), 2);
            QCOMPARE(presses(input)[1].phase, Interaction::Cancel);
            QCOMPARE(presses(input)[0].id, presses(input)[1].id);
        }
    }

    void requestsCanReenterAndDelete() {
        auto item  = std::make_unique<Toggleable>();
        int  count = 0;
        connect(item.get(), &Toggleable::checkedChangeRequested, this, [&](bool value) {
            ++count;
            if (count == 1) {
                item->setChecked(value);
                item->click();
            } else {
                QVERIFY(! value);
                item.reset();
            }
        });
        item->click();
        QVERIFY(! item);
        QCOMPARE(count, 2);
        auto selection = std::make_unique<Selectable>();
        connect(selection.get(), &Selectable::selectionRequested, this, [&] {
            selection.reset();
        });
        key(*selection, QEvent::KeyPress, Qt::Key_Space);
        key(*selection, QEvent::KeyRelease, Qt::Key_Space);
        QVERIFY(! selection);
    }

    void qmlOwner() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                property bool accept: false
                property bool value: false
                property int selection: 0
                MD.Toggleable {
                    objectName: "toggle"
                    checked: parent.value
                    onCheckedChangeRequested: value => { if (parent.accept) parent.value = value; }
                }
                MD.Selectable {
                    objectName: "option"
                    selected: parent.selection === 1
                    onSelectionRequested: { if (parent.accept) parent.selection = 1; }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* toggle = root->findChild<Toggleable*>("toggle");
        auto* option = root->findChild<Selectable*>("option");
        QVERIFY(toggle && option);
        toggle->click();
        option->click();
        QVERIFY(! toggle->isChecked());
        QVERIFY(! option->isSelected());
        root->setProperty("accept", true);
        toggle->click();
        option->click();
        QVERIFY(toggle->isChecked());
        QVERIFY(option->isSelected());
        root->setProperty("value", false);
        root->setProperty("selection", 0);
        QVERIFY(! toggle->isChecked());
        QVERIFY(! option->isSelected());
    }

    void nativePointer() {
        QQuickWindow window;
        window.resize(240, 100);
        Toggleable toggle(window.contentItem());
        Selectable selection(window.contentItem());
        toggle.setSize({ 100, 80 });
        selection.setPosition({ 120, 0 });
        selection.setSize({ 100, 80 });
        QSignalSpy toggleRequests(&toggle, &Toggleable::checkedChangeRequested);
        QSignalSpy selectionRequests(&selection, &Selectable::selectionRequested);
        window.show();
        window.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 50, 40 });
        QCOMPARE(toggleRequests.size(), 1);
        QVERIFY(! toggle.isChecked());
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, { 170, 40 });
        QCOMPARE(selectionRequests.size(), 1);
        QVERIFY(! selection.isSelected());
        selection.forceActiveFocus(Qt::TabFocusReason);
        QTest::keyClick(&window, Qt::Key_Space);
        QCOMPARE(selectionRequests.size(), 2);
        QVERIFY(selection.focusVisible());
    }
};

int run_controlled_input(int argc, char** argv) {
    ControlledInputTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "controlled_input.moc"
