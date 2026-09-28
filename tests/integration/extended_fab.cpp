#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include "qml_material/control/button.hpp"
#include "qml_material/control/action.hpp"

class ExtendedFABTest : public QObject {
    Q_OBJECT
private slots:
    void actionAndContent() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import Qcm.Material as MD
MD.ExtendedFAB {
    action: MD.Action { text: "Create"; icon.name: "add" }
    animationsEnabled: false
}
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* button = qobject_cast<qml_material::Button*>(object.get());
        QVERIFY(button);
        QCOMPARE(button->text(), QStringLiteral("Create"));
        QSignalSpy triggered(button->action(), &qml_material::Action::triggered);
        button->click();
        QCOMPARE(triggered.size(), 1);
        QVERIFY(button->property("expanded").toBool());
        const auto children = button->contentItem()->childItems();
        QCOMPARE(children.size(), 2);
        auto* icon  = children.at(0);
        auto* label = children.at(1);
        QCOMPARE(icon->size(), QSizeF(24, 24));
        QCOMPARE(icon->x(), 16.);
        QCOMPARE(label->x(), 52.);
        const qreal labelWidth = label->width();
        const qreal width      = button->width();
        button->setLayoutDirection(Qt::RightToLeft);
        QCOMPARE(icon->x(), width - 40.);
        QCOMPARE(label->x(), 20.);
        button->setProperty("expansionProgress", .5);
        QCOMPARE(label->width(), labelWidth);
        button->action()->setText(QStringLiteral("Create something new"));
        QVERIFY(button->property("expandedWidth").toReal() > width);
    }
    void transitions() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import Qcm.Material as MD
MD.ExtendedFAB { text: "Create"; icon.name: "add"; expanded: false }
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        object->setProperty("expanded", true);
        QTRY_VERIFY(! object->property("transitioning").toBool());
        QCOMPARE(object->property("expansionProgress").toReal(), 1.);
        object->setProperty("expanded", false);
        object->setProperty("expanded", true);
        QTRY_VERIFY(! object->property("transitioning").toBool());
        QCOMPARE(object->property("expansionProgress").toReal(), 1.);
        object->setProperty("animationsEnabled", false);
        object->setProperty("expanded", false);
        QCOMPARE(object->property("expansionProgress").toReal(), 0.);
        QCOMPARE(object->property("labelOpacity").toReal(), 0.);
    }
    void geometry_data() {
        QTest::addColumn<bool>("expanded");
        QTest::newRow("expanded") << true;
        QTest::newRow("collapsed") << false;
    }
    void geometry() {
        QFETCH(bool, expanded);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import Qcm.Material as MD
MD.ExtendedFAB { text: "Create"; icon.name: "add" }
)",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "expanded", expanded } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(item);
        QCOMPARE(item->height(), 56.);
        QCOMPARE(object->property("transitioning").toBool(), false);
        QCOMPARE(object->property("expansionProgress").toReal(), expanded ? 1. : 0.);
        const qreal natural = object->property("expandedWidth").toReal();
        QVERIFY(natural >= 80.);
        QCOMPARE(item->width(), expanded ? natural : 56.);
        object->setProperty("expansionProgress", .5);
        QCOMPARE(item->width(), (56. + natural) / 2);
        object->setProperty("expansionProgress", 1.);
        QCOMPARE(item->width(), natural);
        object->setProperty("width", 90.);
        QCOMPARE(item->width(), 90.);
        QCOMPARE(object->property("expandedWidth").toReal(), natural);
    }
    void textOnly() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import Qcm.Material as MD
MD.ExtendedFAB { text: "Create"; expanded: false }
)",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* item = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(item);
        QCOMPARE(item->width(), object->property("expandedWidth").toReal());
        QVERIFY(item->width() >= 80.);
    }
};

int run_extended_fab(int argc, char** argv) {
    ExtendedFABTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "extended_fab.moc"
