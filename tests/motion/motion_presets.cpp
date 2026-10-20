#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDir>
#include "qml_material/anim/spring_motion.hpp"

using namespace qml_material;

namespace
{
QAbstractAnimation* clock(QObject* motion) { return motion->findChild<QAbstractAnimation*>(); }
} // namespace

class MotionPresetsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|Cannot assign|TypeError|ReferenceError).*"));
    }

    void qmlParameters_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<qreal>("stiffness");
        QTest::addColumn<qreal>("damping");
        QTest::newRow("fastSpatial") << QString("fastSpatial") << 800.0 << .6;
        QTest::newRow("defaultSpatial") << QString("defaultSpatial") << 380.0 << .8;
        QTest::newRow("slowSpatial") << QString("slowSpatial") << 200.0 << .8;
        QTest::newRow("fastEffects") << QString("fastEffects") << 3800.0 << 1.0;
        QTest::newRow("defaultEffects") << QString("defaultEffects") << 1600.0 << 1.0;
        QTest::newRow("slowEffects") << QString("slowEffects") << 800.0 << 1.0;
    }

    void qmlParameters() {
        QFETCH(QString, name);
        QFETCH(qreal, stiffness);
        QFETCH(qreal, damping);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.SpringMotion {
                property string presetName
                targetValue: 20
                spec: MD.Token.motion.spring.expressive[presetName]
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "presetName", name } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* motion = qobject_cast<SpringMotion*>(object.get());
        QVERIFY(motion);
        const token::SpringSpec expected { stiffness, damping };
        QCOMPARE(motion->spec(), expected);
        QCOMPARE(motion->value(), 20.0);
        QVERIFY(! motion->isRunning());
        QSignalSpy finished(motion, &SpringMotion::finished);
        motion->setTargetValue(80);
        QVERIFY(motion->isRunning());
        auto* animation = clock(motion);
        QVERIFY(animation);
        animation->setCurrentTime(50);
        const auto sample = anim::sampleSpring(expected, 20, 80, 0, .05);
        QVERIFY(qAbs(motion->value() - sample.value) < 1e-9);
        QVERIFY(qAbs(motion->velocity() - sample.velocity) < 1e-9);
        motion->complete();
        QCOMPARE(motion->value(), 80.0);
        QCOMPARE(motion->velocity(), 0.0);
        QVERIFY(! motion->isRunning());
        QCOMPARE(finished.size(), 1);
        motion->complete();
        QCOMPARE(finished.size(), 1);
    }

    void qmlOverrides() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property real targetValue: .6
                property bool animate: true
                readonly property real value: motion.value
                function useEffects() { motion.spec = MD.Token.motion.spring.standard.defaultEffects }
                MD.SpringMotion {
                    id: motion; objectName: "motion"
                    targetValue: root.targetValue
                    enabled: root.animate
                    spec: ({ stiffness: 1800, dampingRatio: .7 })
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* motion = object->findChild<SpringMotion*>("motion");
        QVERIFY(motion);
        QCOMPARE(motion->spec(), (token::SpringSpec { 1800, .7 }));
        QCOMPARE(object->property("value").toReal(), .6);
        QVERIFY(object->setProperty("targetValue", 1.0));
        clock(motion)->setCurrentTime(40);
        const auto value = motion->value(), velocity = motion->velocity();
        QVERIFY(value > .6 && value < 1);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "useEffects"));
        QCOMPARE(motion->spec(), (token::SpringSpec { 1600, 1 }));
        QCOMPARE(motion->value(), value);
        QCOMPARE(motion->velocity(), velocity);
        QVERIFY(object->setProperty("targetValue", 0.0));
        QCOMPARE(motion->value(), value);
        QCOMPARE(motion->velocity(), velocity);
        motion->complete();
        QCOMPARE(object->property("value").toReal(), 0.0);
        QVERIFY(object->setProperty("animate", false));
        QVERIFY(object->setProperty("targetValue", .8));
        QCOMPARE(object->property("value").toReal(), .8);
        QVERIFY(! motion->isRunning());
        QVERIFY(object->setProperty("animate", true));
        QVERIFY(object->setProperty("targetValue", .4));
        clock(motion)->setCurrentTime(30);
        QVERIFY(object->setProperty("animate", false));
        QCOMPARE(object->property("value").toReal(), .4);
        QVERIFY(! motion->isRunning());
    }
};

int run_motion_presets(int argc, char** argv) {
    MotionPresetsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "motion_presets.moc"
