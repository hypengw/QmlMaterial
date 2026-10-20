#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/button_group.hpp"
#include "qml_material/control/radio_button.hpp"
#include "qml_material/control/tab_bar.hpp"

using namespace qml_material;
namespace
{
ButtonGroupAttached* projection(ToggleButton* button) {
    return qobject_cast<ButtonGroupAttached*>(
        qmlAttachedPropertiesObject<ButtonGroup>(button, true));
}
void attach(ButtonGroup& group, ToggleButton& button) {
    auto* source = projection(&button);
    source->setGroup(&group);
    button.bindableChecked().setBinding([source] {
        return source->isChecked();
    });
}
} // namespace

class ButtonSelectionTest : public QObject {
    Q_OBJECT

private slots:
    void init() { QTest::failOnWarning(QRegularExpression(".*")); }

    void atomicSelection() {
        ToggleButton a, b, c;
        ButtonGroup  group;
        for (auto* button : { &a, &b, &c }) attach(group, *button);
        group.setCheckedButton(&a);
        int        notifications = 0;
        const auto verify        = [&] {
            ++notifications;
            QCOMPARE(group.checkedButton(), &b);
            QVERIFY(! a.isChecked() && b.isChecked() && ! c.isChecked());
            QVERIFY(! a.bindableChecked().value() && b.bindableChecked().value());
            QCOMPARE(group.checkState(), Qt::PartiallyChecked);
        };
        for (auto* button : { &a, &b, &c })
            connect(button, &ToggleButton::checkedChanged, this, verify);
        connect(&group, &ButtonGroup::checkedButtonsChanged, this, verify);
        b.click();
        QCOMPARE(notifications, 3);
        b.click();
        QCOMPARE(notifications, 3);
        for (auto* button : { &a, &b, &c }) disconnect(button, nullptr, this, nullptr);
        disconnect(&group, nullptr, this, nullptr);
    }

    void multipleSelectionIsAtomic() {
        ToggleButton a, b, c;
        ButtonGroup  group;
        group.setExclusive(false);
        for (auto* button : { &a, &b, &c }) attach(group, *button);
        bool expected      = true;
        int  notifications = 0;
        for (auto* button : { &a, &b, &c })
            connect(button, &ToggleButton::checkedChanged, this, [&] {
                ++notifications;
                QCOMPARE(a.isChecked(), expected);
                QCOMPARE(b.isChecked(), expected);
                QCOMPARE(c.isChecked(), expected);
                QCOMPARE(group.checkState(), expected ? Qt::Checked : Qt::Unchecked);
            });
        group.setCheckState(Qt::Checked);
        QCOMPARE(notifications, 3);
        expected = false;
        group.setCheckState(Qt::Unchecked);
        QCOMPARE(notifications, 6);
    }

    void rejectedRequest() {
        QQuickWindow window;
        ToggleButton a(window.contentItem()), outsider;
        ButtonGroup  group;
        attach(group, a);
        a.setEnabled(false);
        a.click();
        QVERIFY(! a.isChecked());
        a.setEnabled(true);
        a.setVisible(false);
        a.click();
        QVERIFY(! a.isChecked());
        a.setVisible(true);
        group.requestChecked(&outsider, true);
        QVERIFY(! group.checkedButton());
        a.click();
        QVERIFY(a.isChecked());
        a.click();
        QVERIFY(a.isChecked());
        group.setCheckedButton(nullptr);
        QVERIFY(! a.isChecked());
    }

    void reentrantSelection() {
        ToggleButton a, b, c;
        ButtonGroup  group;
        for (auto* button : { &a, &b, &c }) attach(group, *button);
        group.setCheckedButton(&a);
        connect(&a, &ToggleButton::checkedChanged, this, [&] {
            if (! a.isChecked()) group.setCheckedButton(&c);
        });
        group.setCheckedButton(&b);
        QCOMPARE(group.checkedButton(), &c);
        QVERIFY(! a.isChecked() && ! b.isChecked() && c.isChecked());
        QVERIFY(a.bindableChecked().hasBinding() && b.bindableChecked().hasBinding());
    }

    void reentrantBindableObserver() {
        ToggleButton a, b, c;
        ButtonGroup  group;
        for (auto* button : { &a, &b, &c }) attach(group, *button);
        group.setCheckedButton(&a);
        const auto observer = a.bindableChecked().addNotifier([&] {
            if (! a.isChecked()) group.setCheckedButton(&c);
        });
        group.setCheckedButton(&b);
        QCOMPARE(group.checkedButton(), &c);
        QVERIFY(! a.isChecked() && ! b.isChecked() && c.isChecked());
    }

    void groupDeletedInBindableObserver() {
        ToggleButton a, b;
        auto         group = std::make_unique<ButtonGroup>();
        attach(*group, a);
        attach(*group, b);
        group->setCheckedButton(&a);
        const auto observer = a.bindableChecked().addNotifier([&] {
            group.reset();
        });
        group->setCheckedButton(&b);
        QVERIFY(! group);
        QVERIFY(! a.isChecked() && ! b.isChecked());
        QVERIFY(! projection(&a)->group() && ! projection(&b)->group());
    }

    void deletionDuringNotification_data() {
        QTest::addColumn<bool>("removeGroup");
        QTest::newRow("member") << false;
        QTest::newRow("group") << true;
    }

    void deletionDuringNotification() {
        QFETCH(bool, removeGroup);
        ToggleButton a;
        auto         b     = std::make_unique<ToggleButton>();
        auto         group = std::make_unique<ButtonGroup>();
        attach(*group, a);
        attach(*group, *b);
        group->setCheckedButton(&a);
        connect(&a, &ToggleButton::checkedChanged, this, [&] {
            if (removeGroup)
                group.reset();
            else
                b.reset();
        });
        group->setCheckedButton(b.get());
        QVERIFY(! a.isChecked());
        if (removeGroup) {
            QVERIFY(! group);
            QVERIFY(! b->isChecked());
            QVERIFY(! projection(&a)->group() && ! projection(b.get())->group());
        } else {
            QVERIFY(! b);
            QVERIFY(! group->checkedButton());
            QCOMPARE(group->checkState(), Qt::Unchecked);
        }
    }

    void atomicTeardown() {
        ToggleButton a, b;
        auto         group = std::make_unique<ButtonGroup>();
        attach(*group, a);
        attach(*group, b);
        group->setExclusive(false);
        group->setCheckState(Qt::Checked);
        int        notifications = 0;
        const auto verify        = [&] {
            ++notifications;
            QVERIFY(! a.isChecked() && ! b.isChecked());
        };
        connect(&a, &ToggleButton::checkedChanged, this, verify);
        connect(&b, &ToggleButton::checkedChanged, this, verify);
        group.reset();
        QCOMPARE(notifications, 2);
        QVERIFY(! projection(&a)->group() && ! projection(&b)->group());
    }

    void moveBetweenGroups() {
        ToggleButton a, b;
        ButtonGroup  first, second;
        attach(first, a);
        attach(second, b);
        first.setCheckedButton(&a);
        second.setCheckedButton(&b);
        second.addButton(&a);
        QCOMPARE(projection(&a)->group(), &second);
        QVERIFY(! first.checkedButton());
        QCOMPARE(second.checkedButton(), &b);
        QVERIFY(! a.isChecked() && b.isChecked());
        second.setCheckedButton(&a);
        QVERIFY(a.isChecked() && ! b.isChecked());
        second.removeButton(&a);
        QVERIFY(! a.isChecked());
        QVERIFY(! second.checkedButton());
        QVERIFY(! projection(&a)->group());
    }

    void retargetCancelsActivation_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::addColumn<int>("change");
        for (bool keyboard : { false, true })
            for (int change = 0; change < 3; ++change)
                QTest::newRow(qPrintable(QString("%1-%2").arg(keyboard).arg(change)))
                    << keyboard << change;
    }

    void retargetCancelsActivation() {
        QFETCH(bool, keyboard);
        QFETCH(int, change);
        QQuickWindow window;
        window.resize(200, 100);
        ToggleButton button(window.contentItem());
        button.setSize({ 100, 50 });
        auto        first = std::make_unique<ButtonGroup>();
        ButtonGroup second;
        attach(*first, button);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (keyboard) {
            button.forceActiveFocus(Qt::TabFocusReason);
            QTest::keyPress(&window, Qt::Key_Space);
        } else {
            QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        }
        QSignalSpy requests(&button, &ToggleButton::checkedChangeRequested);
        if (change == 0)
            second.addButton(&button);
        else if (change == 1)
            first->removeButton(&button);
        else
            first.reset();
        if (keyboard)
            QTest::keyRelease(&window, Qt::Key_Space);
        else
            QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, { 20, 20 });
        QVERIFY(requests.isEmpty());
        QVERIFY(! button.isChecked());
        second.addButton(&button);
        button.click();
        QCOMPARE(requests.count(), 1);
        QCOMPARE(second.checkedButton(), &button);
        QVERIFY(button.isChecked());
    }

    void exclusivePolicy() {
        ToggleButton a, b, c;
        ButtonGroup  group;
        for (auto* button : { &a, &b, &c }) attach(group, *button);
        group.setExclusive(false);
        group.setCheckedButtons({ &b, &c });
        QVERIFY(! group.checkedButton());
        QVERIFY(! a.isChecked() && b.isChecked() && c.isChecked());
        group.setExclusive(true);
        QCOMPARE(group.checkedButton(), &b);
        QVERIFY(! a.isChecked() && b.isChecked() && ! c.isChecked());
        QSignalSpy changes(&group, &ButtonGroup::checkedButtonsChanged);
        group.setExclusive(false);
        QCOMPARE(changes.count(), 1);
        QVERIFY(! group.checkedButton());
        QVERIFY(b.isChecked());
    }

    void retargetInRequestObserver() {
        ToggleButton button;
        ButtonGroup  first, second;
        connect(&button, &ToggleButton::checkedChangeRequested, this, [&] {
            second.addButton(&button);
        });
        attach(first, button);
        button.click();
        QVERIFY(! first.checkedButton() && ! second.checkedButton());
        QVERIFY(! button.isChecked());
        button.click();
        QCOMPARE(second.checkedButton(), &button);
        QVERIFY(button.isChecked());
    }

    void explicitProjection() {
        ToggleButton a, b;
        QVERIFY(projection(&a)->bindableChecked().isReadOnly());
        QProperty<bool> external(true);
        a.bindableChecked().setBinding([&] {
            return external.value();
        });
        ButtonGroup group;
        group.addButton(&a);
        group.addButton(&b);
        group.setCheckedButton(&b);
        QVERIFY(a.isChecked());
        QVERIFY(! b.isChecked());
        QVERIFY(projection(&b)->isChecked());
        QVERIFY(a.bindableChecked().hasBinding());
        external = false;
        QVERIFY(! a.isChecked());
        QCOMPARE(group.checkedButton(), &b);
    }

    void qmlInitialSelection() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                MD.ButtonGroup {
                    id: selection
                    objectName: "selection"
                    checkedButtons: [second]
                }
                MD.ToggleButton {
                    objectName: "first"
                    MD.ButtonGroup.group: selection
                    checked: MD.ButtonGroup.checked
                }
                MD.IconToggleButton {
                    id: second
                    objectName: "second"
                    MD.ButtonGroup.group: selection
                    checked: MD.ButtonGroup.checked
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* group  = object->findChild<ButtonGroup*>("selection");
        auto* first  = object->findChild<ToggleButton*>("first");
        auto* second = object->findChild<ToggleButton*>("second");
        QVERIFY(group && first && second);
        QCOMPARE(group->checkedButton(), second);
        QVERIFY(! first->isChecked() && second->isChecked());
        first->click();
        QVERIFY(first->isChecked() && ! second->isChecked());
    }

    void radioSelectionHasExplicitOwner() {
        QQuickItem     parent;
        QProperty<int> owner(-1);
        RadioButton    a(&parent), b(&parent);
        a.click();
        b.click();
        QVERIFY(! a.isSelected() && ! b.isSelected());
        QCOMPARE(a.metaObject()->indexOfProperty("autoExclusive"), -1);
        a.bindableSelected().setBinding([&] {
            return owner.value() == 0;
        });
        b.bindableSelected().setBinding([&] {
            return owner.value() == 1;
        });
        connect(&a, &SelectionButton::selectionRequested, this, [&] {
            owner = 0;
        });
        connect(&b, &SelectionButton::selectionRequested, this, [&] {
            owner = 1;
        });
        b.click();
        QVERIFY(! a.isSelected() && b.isSelected());
        a.click();
        QVERIFY(a.isSelected() && ! b.isSelected());
    }

    void tabDeselectionAndRemoval() {
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.TabBar {
                width: 300; height: 48
                MD.TabButton { text: "A" }
                MD.TabButton { text: "B" }
                MD.TabButton { text: "C" }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<TabBar*>(object.get());
        QVERIFY(bar);
        bar->setCurrentIndex(1);
        bar->setCurrentIndex(-1);
        QCOMPARE(bar->currentIndex(), -1);
        QVERIFY(! bar->currentItem());
        for (int i = 0; i < bar->count(); ++i) QTRY_VERIFY(! bar->itemAt(i)->isSelected());
        bar->setCurrentIndex(1);
        auto*                       next = bar->itemAt(2);
        std::unique_ptr<QQuickItem> removed(bar->takeItem(1));
        QCOMPARE(bar->currentIndex(), 1);
        QCOMPARE(bar->currentItem(), next);
        QTRY_VERIFY(next->isSelected());
        QTRY_VERIFY(! bar->itemAt(0)->isSelected());
    }

    void tabSelectionIsAtomic_data() {
        QTest::addColumn<bool>("reenter");
        QTest::newRow("normal") << false;
        QTest::newRow("reentrant") << true;
    }

    void tabSelectionIsAtomic() {
        QFETCH(bool, reenter);
        QQmlEngine    engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.TabBar {
                currentIndex: 0
                width: 300; height: 48
                MD.TabButton { text: "A" }
                MD.TabButton { text: "B" }
                MD.TabButton { text: "C" }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<TabBar*>(object.get());
        QVERIFY(bar);
        bar->setCurrentIndex(0);
        int expected = 1;
        if (reenter)
            connect(bar, &Container::currentIndexChanged, this, [&] {
                if (bar->currentIndex() == 1) {
                    expected = 2;
                    bar->setCurrentIndex(2);
                    QCOMPARE(bar->currentIndex(), 2);
                }
            });
        const auto verify = [&] {
            QCOMPARE(bar->currentIndex(), expected);
            QCOMPARE(bar->currentItem(), bar->itemAt(expected));
        };
        connect(bar, &Container::currentIndexChanged, this, verify);
        connect(bar, &Container::currentItemChanged, this, verify);
        bar->setCurrentIndex(1);
        QCOMPARE(bar->currentIndex(), reenter ? 2 : 1);
        for (int i = 0; i < 3; ++i) QTRY_COMPARE(bar->itemAt(i)->isSelected(), i == expected);
        expected = reenter ? 0 : 2;
        bar->setCurrentIndex(expected);
        verify();
        for (int i = 0; i < 3; ++i) QTRY_COMPARE(bar->itemAt(i)->isSelected(), i == expected);
    }

    void buttonGroup() {
        ToggleButton a, b;
        ButtonGroup  group;
        auto*        first =
            qobject_cast<ButtonGroupAttached*>(qmlAttachedPropertiesObject<ButtonGroup>(&a, true));
        auto* second =
            qobject_cast<ButtonGroupAttached*>(qmlAttachedPropertiesObject<ButtonGroup>(&b, true));
        first->setGroup(&group);
        second->setGroup(&group);
        a.bindableChecked().setBinding([first] {
            return first->isChecked();
        });
        b.bindableChecked().setBinding([second] {
            return second->isChecked();
        });
        group.setCheckedButton(&a);
        b.click();
        QVERIFY(! a.isChecked() && b.isChecked());
        QVERIFY(a.bindableChecked().hasBinding() && b.bindableChecked().hasBinding());
        group.setCheckedButton(&a);
        QVERIFY(a.isChecked() && ! b.isChecked());
        QVERIFY(a.bindableChecked().hasBinding() && b.bindableChecked().hasBinding());
    }

    void explicitExclusiveGroup() {
        ToggleButton    a, b;
        QProperty<bool> first(true), second(false);
        a.bindableChecked().setBinding([&] {
            return first.value();
        });
        b.bindableChecked().setBinding([&] {
            return second.value();
        });
        ButtonGroup group;
        group.addButton(&a);
        group.addButton(&b);
        b.click();
        QCOMPARE(group.checkedButton(), &b);
        QVERIFY(a.isChecked() && ! b.isChecked());
        QVERIFY(a.bindableChecked().hasBinding() && b.bindableChecked().hasBinding());
        first  = false;
        second = true;
        QVERIFY(! a.isChecked() && b.isChecked());
    }
};

int run_button_selection(int argc, char** argv) {
    ButtonSelectionTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "button_selection.moc"
