#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include "qml_material/control/control.hpp"

class FloatingToolbarTest : public QObject {
    Q_OBJECT
    QQmlEngine m_engine;
private slots:
    void initTestCase() { m_engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH)); }
    void geometry_data() {
        QTest::addColumn<bool>("vertical");
        QTest::addColumn<bool>("mirrored");
        QTest::newRow("horizontal") << false << false;
        QTest::newRow("rtl") << false << true;
        QTest::newRow("vertical") << true << false;
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
