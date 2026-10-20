#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
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

class ExampleSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void examples_data() {
        QTest::addColumn<QString>("path");
        for (const auto* name : { "Components.qml", "Shape.qml" })
            QTest::newRow(name) << QString::fromLatin1(name);
    }

    void examples() {
        QFETCH(QString, path);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent            component(&engine, QUrl("qrc:/Qcm/Material/Example/" + path));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "width", 1280 }, { "height", 900 } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        QCoreApplication::processEvents();
    }
};

int run_example_smoke(int argc, char** argv) {
    ExampleSmokeTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "example_smoke.moc"
