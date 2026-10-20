#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include "qml_material/control/combo_box.hpp"
#include "qml_material/control/navigation_rail.hpp"
#include "qml_material/control/menu.hpp"
#include "qml_material/control/item_delegate.hpp"
#include "qml_material/control/range_slider.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/control/action_command.hpp"
#include "qml_material/style/slider_state.hpp"
#include "qml_material/style/combo_box_state.hpp"
#include "../input_test.hpp"

using namespace qml_material;
namespace
{
void mouse(QQuickItem* item, QEvent::Type type, QPointF point) {
    QMouseEvent event(type,
                      point,
                      point,
                      Qt::LeftButton,
                      type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(item, &event);
    flushInteractions();
}
} // namespace

class DelegateInputTest : public QObject {
    Q_OBJECT

private slots:
    void variants_data() {
        QTest::addColumn<QByteArray>("type");
        QTest::addColumn<bool>("toggle");
        QTest::addColumn<bool>("menu");
        for (bool menu : { false, true })
            for (bool toggle : { false, true }) {
                const QByteArray name =
                    QByteArray(toggle ? "Toggle" : "Selection") + (menu ? "MenuItem" : "ListItem");
                QTest::newRow(name) << name << toggle << menu;
            }
    }

    void variants() {
        QFETCH(QByteArray, type);
        QFETCH(bool, toggle);
        QFETCH(bool, menu);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick; import Qcm.Material as MD; MD." + type +
                              " { width: 240; text: 'Choice' }",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<ButtonControl*>(object.get());
        QVERIFY(button);
        const char* property = toggle ? "checked" : "selected";
        QSignalSpy  requested(
            button, toggle ? SIGNAL(checkedChangeRequested(bool)) : SIGNAL(selectionRequested()));
        button->click();
        QCOMPARE(requested.size(), 1);
        QVERIFY(! button->property(property).toBool());
        QVERIFY(button->setProperty(property, true));
        QVERIFY(button->property(property).toBool());
        auto* state = button->findChild<CommonState*>();
        QVERIFY(state);
        QVERIFY(state->property("selected").toBool());
        if (menu) {
            Menu owner;
            owner.setParentItem(button);
            owner.addItem(button);
            QSignalSpy triggers(button, SIGNAL(triggered()));
            button->click();
            QCOMPARE(triggers.size(), 1);
            QCOMPARE(owner.itemAt(0), button);
            QCOMPARE(owner.takeItem(0), button);
        }
    }

    void menuRetarget() {
        Menu              first, second;
        SelectionMenuItem item;
        item.setSize({ 200, 48 });
        first.addItem(&item);
        QSignalSpy requests(&item, &SelectionButton::selectionRequested);
        mouse(&item, QEvent::MouseButtonPress, { 20, 20 });
        second.addItem(&item);
        QCOMPARE(first.count(), 0);
        QCOMPARE(second.count(), 1);
        mouse(&item, QEvent::MouseButtonRelease, { 20, 20 });
        QCOMPARE(requests.size(), 0);
        connect(&item, &SelectionButton::selectionRequested, &item, [&] {
            item.setMenu(nullptr);
        });
        QSignalSpy triggered(&item, &SelectionMenuItem::triggered);
        item.click();
        QCOMPARE(requests.size(), 1);
        QCOMPARE(triggered.size(), 0);
    }

    void deletion_data() {
        QTest::addColumn<int>("type");
        for (int i = 0; i < 2; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }

    void deletion() {
        QFETCH(int, type);
        if (type == 0) {
            auto*                       item = new SelectionMenuItem;
            QPointer<SelectionMenuItem> guard(item);
            connect(item, &SelectionButton::selectionRequested, item, [item] {
                delete item;
            });
            item->click();
            QVERIFY(! guard);
        } else if (type == 1) {
            auto*                    item = new ToggleMenuItem;
            QPointer<ToggleMenuItem> guard(item);
            connect(item, &ToggleButton::checkedChangeRequested, item, [item] {
                delete item;
            });
            item->click();
            QVERIFY(! guard);
        }
    }

    void combinedCommands() {
        ItemDelegate list;
        MenuItem     menu;
        for (auto* item : { static_cast<CombinedCommandButton*>(&list),
                            static_cast<CombinedCommandButton*>(&menu) }) {
            QVERIFY(! item->longClickEnabled());
            QVERIFY(! item->doubleClickEnabled());
            item->setLongClickEnabled(true);
            item->setLongClickInterval(50);
            QCOMPARE(item->longClickInterval(), 50);
            item->setDoubleClickEnabled(true);
        }
    }
};

int run_delegate_input(int argc, char** argv) {
    DelegateInputTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "delegate_input.moc"
