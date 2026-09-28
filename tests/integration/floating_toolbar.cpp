#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include "qml_material/control/control.hpp"
#include "qml_material/input/floating_toolbar_scroll.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include <limits>

class FloatingToolbarTest : public QObject {
    Q_OBJECT
    QQmlEngine m_engine;
private slots:
    void initTestCase() { m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH)); }
    void scrollThresholds() {
        qml_material::FloatingToolbarScroll behavior;
        QSignalSpy collapse(&behavior, &qml_material::FloatingToolbarScroll::collapseRequested);
        QSignalSpy expand(&behavior, &qml_material::FloatingToolbarScroll::expandRequested);
        connect(&behavior, &qml_material::FloatingToolbarScroll::collapseRequested, &behavior, [&] {
            behavior.setExpanded(false);
        });
        connect(&behavior, &qml_material::FloatingToolbarScroll::expandRequested, &behavior, [&] {
            behavior.setExpanded(true);
        });
        behavior.scrollBy({ 100, 0 });
        behavior.scrollBy({ 0, 39 });
        QCOMPARE(collapse.size(), 0);
        behavior.scrollBy({ 0, 1 });
        QCOMPARE(collapse.size(), 1);
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, 300 });
        QCOMPARE(collapse.size(), 1);
        behavior.scrollBy({ 0, -20 });
        behavior.scrollBy({ 0, 10 });
        behavior.scrollBy({ 0, -39 });
        QCOMPARE(expand.size(), 0);
        behavior.scrollBy({ 0, -1 });
        QCOMPARE(expand.size(), 1);
        behavior.setCollapseScrollThreshold(60);
        behavior.setExpandScrollThreshold(20);
        behavior.scrollBy({ 0, 59 });
        behavior.setEnabled(false);
        behavior.scrollBy({ 0, 100 });
        behavior.setEnabled(true);
        behavior.scrollBy({ 0, 59 });
        QVERIFY(behavior.expanded());
        behavior.scrollBy({ 0, 1 });
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, -20 });
        QVERIFY(behavior.expanded());
        behavior.setReverseLayout(true);
        behavior.scrollBy({ 0, -60 });
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, 20 });
        QVERIFY(behavior.expanded());
        behavior.scrollBy({ 0, -59 });
        behavior.reset();
        behavior.scrollBy({ 0, -1 });
        QVERIFY(behavior.expanded());
        behavior.setExpanded(false);
        behavior.scrollBy({ 0, 19 });
        QVERIFY(! behavior.expanded());
        behavior.scrollBy({ 0, std::numeric_limits<qreal>::quiet_NaN() });
        behavior.setExpandScrollThreshold(-1);
        QCOMPARE(behavior.expandScrollThreshold(), 20.);
        behavior.scrollBy({ 0, 1 });
        QVERIFY(behavior.expanded());
    }
    void geometry_data() {
        QTest::addColumn<bool>("vertical");
        QTest::addColumn<bool>("mirrored");
        QTest::newRow("horizontal") << false << false;
        QTest::newRow("rtl") << false << true;
        QTest::newRow("vertical") << true << false;
    }
    void scrollBinding() {
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    id: bar
    animationsEnabled: false
    property MD.FloatingToolbarScroll behavior: MD.FloatingToolbarScroll {
        expanded: bar.expanded
        onCollapseRequested: bar.expanded = false
        onExpandRequested: bar.expanded = true
    }
    property MD.Scrollable scrollSource: MD.Scrollable {
        MD.NestedScroll.enabled: true
        MD.NestedScroll.onScrollConsumed: delta => bar.behavior.scrollBy(delta)
    }
    mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
    trailingContent: Item { implicitWidth: 48; implicitHeight: 48 }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* behavior =
            qvariant_cast<qml_material::FloatingToolbarScroll*>(object->property("behavior"));
        QVERIFY(behavior);
        auto* source = qvariant_cast<QObject*>(object->property("scrollSource"));
        QVERIFY(source);
        auto* nested = qobject_cast<qml_material::NestedScroll*>(
            qmlAttachedPropertiesObject<qml_material::NestedScroll>(source));
        QVERIFY(nested);
        emit nested->scrollConsumed({ 0, 40 });
        QVERIFY(! object->property("expanded").toBool());
        QVERIFY(! behavior->expanded());
        QCOMPARE(object->property("width").toReal(), 64.);
        object->setProperty("expanded", true);
        QVERIFY(behavior->expanded());
        behavior->scrollBy({ 0, 40 });
        QVERIFY(! behavior->expanded());
        behavior->scrollBy({ 0, -40 });
        QVERIFY(object->property("expanded").toBool());
        QCOMPARE(object->property("width").toReal(), 112.);
    }
    void geometry() {
        QFETCH(bool, vertical);
        QFETCH(bool, mirrored);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    animationsEnabled: false
    leadingContent: Item { implicitWidth: 48; implicitHeight: 48 }
    mainContent: Item { implicitWidth: 48; implicitHeight: 48 }
    trailingContent: Item { implicitWidth: 48; implicitHeight: 48 }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* bar = qobject_cast<qml_material::Control*>(object.get());
        QVERIFY(bar);
        bar->setProperty("orientation", vertical ? Qt::Vertical : Qt::Horizontal);
        bar->setLayoutDirection(mirrored ? Qt::RightToLeft : Qt::LeftToRight);
        auto* main     = qvariant_cast<QQuickItem*>(bar->property("mainContent"));
        auto* leading  = qvariant_cast<QQuickItem*>(bar->property("leadingContent"));
        auto* trailing = qvariant_cast<QQuickItem*>(bar->property("trailingContent"));
        QVERIFY(main && leading && trailing);
        QCOMPARE(bar->size(), vertical ? QSizeF(64, 160) : QSizeF(160, 64));
        auto position = [&](QQuickItem* item) {
            const auto point = item->mapToItem(bar, QPointF());
            return vertical ? point.y() : point.x();
        };
        QCOMPARE(position(main), 56.);
        QCOMPARE(position(leading), mirrored ? 104. : 8.);
        QCOMPARE(position(trailing), mirrored ? 8. : 104.);
        bar->setProperty("expanded", false);
        QCOMPARE(bar->size(), QSizeF(64, 64));
        QCOMPARE(main->size(), QSizeF(48, 48));
        QVERIFY(! leading->isVisible());
        QVERIFY(! leading->isEnabled());
        QVERIFY(main->isVisible());
        bar->setProperty("expanded", true);
        bar->setProperty("expansionProgress", .5);
        QCOMPARE(bar->size(), vertical ? QSizeF(64, 112) : QSizeF(112, 64));
        QCOMPARE(leading->size(), QSizeF(48, 48));
        QCOMPARE(main->size(), QSizeF(48, 48));
        leading->setImplicitWidth(72);
        leading->setImplicitHeight(72);
        QCOMPARE(vertical ? bar->height() : bar->width(), 124.);
        if (vertical)
            bar->setHeight(48);
        else
            bar->setWidth(48);
        QCOMPARE(vertical ? main->height() : main->width(), 32.);
        QVERIFY(position(main) >= 8.);
        QVERIFY(! leading->isVisible());
        QVERIFY(! trailing->isVisible());
        bar->setProperty("leadingContent", QVariant::fromValue<QQuickItem*>(nullptr));
        QVERIFY(! leading->parentItem());
        QCOMPARE(qvariant_cast<QQuickItem*>(bar->property("mainContent")), main);
        delete trailing;
        QVERIFY(! qvariant_cast<QQuickItem*>(bar->property("trailingContent")));
    }
    void actionToolbar() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    width: 112
    mainContent: MD.ActionToolBar {
        actions: [
            MD.Action { icon.name: "edit" },
            MD.Action { icon.name: "share" },
            MD.Action { icon.name: "download" }
        ]
    }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* main = qvariant_cast<QQuickItem*>(object->property("mainContent"));
        QVERIFY(main);
        auto polish = [](auto&& self, QQuickItem* item) -> void {
            item->ensurePolished();
            for (auto* child : item->childItems()) self(self, child);
        };
        polish(polish, qobject_cast<QQuickItem*>(object.get()));
        QCoreApplication::processEvents();
        QCOMPARE(main->width(), 96.);
        QVERIFY(main->property("maximumContentWidth").toReal() > main->width());
        QVERIFY(main->property("visibleWidth").toReal() <= main->width());
        object->setProperty("expanded", false);
        QCOMPARE(qvariant_cast<QQuickItem*>(object->property("mainContent")), main);
        object->setProperty("mainContent", QVariant::fromValue<QQuickItem*>(nullptr));
        object->setProperty("trailingContent", QVariant::fromValue(main));
        object->setProperty("animationsEnabled", false);
        object->setProperty("width", 300.);
        object->setProperty("expanded", true);
        polish(polish, qobject_cast<QQuickItem*>(object.get()));
        QTRY_VERIFY(main->isVisible());
        QVERIFY(main->implicitWidth() >= 144.);
        object->setProperty("expanded", false);
        QVERIFY(! main->isVisible());
        object->setProperty("expanded", true);
        polish(polish, qobject_cast<QQuickItem*>(object.get()));
        QVERIFY(main->isVisible());
        QVERIFY(main->implicitWidth() >= 144.);
    }

    void transitions() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
MD.FloatingToolbar {
    expanded: false
    mainContent: MD.IconButton { icon.name: "edit" }
    trailingContent: MD.IconButton { icon.name: "share" }
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("expansionProgress").toReal(), 0.);
        QVERIFY(! object->property("transitioning").toBool());
        object->setProperty("expanded", true);
        object->setProperty("expanded", false);
        QTRY_VERIFY(! object->property("transitioning").toBool());
        QCOMPARE(object->property("expansionProgress").toReal(), 0.);
        object->setProperty("expanded", true);
        QTRY_VERIFY(! object->property("transitioning").toBool());
        QCOMPARE(object->property("expansionProgress").toReal(), 1.);
    }
};

int run_floating_toolbar(int argc, char** argv) {
    FloatingToolbarTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "floating_toolbar.moc"
