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

class NavigationRailTest : public QObject {
    Q_OBJECT

private slots:
    void railTargets() {
        NavigationRail rail;
        rail.setUseEmbed(true);
        QVERIFY(rail.expanded());
        rail.close();
        QVERIFY(! rail.expanded());
        rail.open();
        rail.open();
        QVERIFY(rail.expanded());
        rail.defaultHeaderAction()->trigger();
        QVERIFY(! rail.expanded());
        rail.setAutoExpand(false);
        rail.setUseEmbed(false);
        rail.setUseEmbed(true);
        QVERIFY(! rail.expanded());
        rail.setAutoExpand(true);
        QVERIFY(rail.expanded());
        rail.setForceModal(true);
        QVERIFY(rail.useModal());
        rail.setEnabled(false);
        rail.defaultHeaderAction()->trigger();
        QVERIFY(! rail.drawerRequested());
    }

    void railCancelsHeader_data() {
        QTest::addColumn<int>("change");
        for (int i = 0; i < 7; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }

    void railCancelsHeader() {
        QFETCH(int, change);
        NavigationRail rail;
        rail.setUseEmbed(true);
        CommandButton header, replacement;
        header.setSize({ 48, 48 });
        rail.setHeaderControl(&header);
        auto* command = ActionCommand::qmlAttachedProperties(&header);
        command->setAction(rail.defaultHeaderAction());
        QSignalSpy trigger(rail.defaultHeaderAction(), &Action::triggered);
        mouse(&header, QEvent::MouseButtonPress, { 20, 20 });
        switch (change) {
        case 0: rail.setHeaderControl(&replacement); break;
        case 1: rail.setUseEmbed(false); break;
        case 2: rail.setExpanded(false); break;
        case 3: rail.setForceModal(true); break;
        case 4: rail.setEnabled(false); break;
        case 5: rail.setVisible(false); break;
        case 6: rail.setPopupSite(new PresentationSite(&rail)); break;
        }
        mouse(&header, QEvent::MouseButtonRelease, { 20, 20 });
        QCOMPARE(trigger.size(), 0);
    }

    void railQml() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.NavigationRail { useEmbed: true; width: 220; height: 400 }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* rail = qobject_cast<NavigationRail*>(object.get());
        QVERIFY(rail);
        QQuickWindow window;
        window.resize(500, 500);
        rail->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(rail->headerControl() && rail->headerControl()->isVisible());
        QVERIFY(rail->expanded());
        rail->headerControl()->click();
        QVERIFY(! rail->expanded());
        rail->headerControl()->click();
        QVERIFY(rail->expanded());
        rail->setUseEmbed(false);
        rail->open();
        QTRY_VERIFY(rail->drawerRequested());
        QTRY_VERIFY(rail->drawer()->isVisible());
        rail->close();
        QTRY_VERIFY(! rail->drawerRequested());
    }

    void deletedInExpandedNotification() {
        auto* rail = new NavigationRail;
        rail->setUseEmbed(true);
        QPointer<NavigationRail> guard(rail);
        connect(rail, &NavigationRail::expandedChanged, rail, [rail] {
            delete rail;
        });
        rail->toggle();
        QVERIFY(! guard);
    }
};

int run_navigation_rail(int argc, char** argv) {
    NavigationRailTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "navigation_rail.moc"
