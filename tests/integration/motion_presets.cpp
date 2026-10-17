#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <private/qquickanimation_p.h>
#include "qml_material/token/motion.hpp"

using namespace qml_material;

namespace
{
token::MotionPreset preset(const QString& name) {
    const auto expressive = token::Motion {}.expressive();
    if (name == "fastSpatial") return expressive.fastSpatial();
    if (name == "defaultSpatial") return expressive.defaultSpatial();
    if (name == "slowSpatial") return expressive.slowSpatial();
    return expressive.defaultEffects();
}

void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}
} // namespace

class MotionPresetsTest : public QObject {
    Q_OBJECT
private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|Cannot assign|TypeError|ReferenceError).*"));
    }
    void parameters_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("duration");
        QTest::addColumn<QPointF>("first");
        QTest::addColumn<QPointF>("second");
        QTest::addColumn<bool>("spatial");
        QTest::newRow("fastSpatial")
            << QString("fastSpatial") << 350 << QPointF(.42, 1.67) << QPointF(.21, .90) << true;
        QTest::newRow("defaultSpatial")
            << QString("defaultSpatial") << 500 << QPointF(.38, 1.21) << QPointF(.22, 1) << true;
        QTest::newRow("slowSpatial")
            << QString("slowSpatial") << 650 << QPointF(.39, 1.29) << QPointF(.35, .98) << true;
        QTest::newRow("defaultEffects")
            << QString("defaultEffects") << 200 << QPointF(.34, .80) << QPointF(.34, 1) << false;
    }
    void parameters() {
        QFETCH(QString, name);
        QFETCH(int, duration);
        QFETCH(QPointF, first);
        QFETCH(QPointF, second);
        QFETCH(bool, spatial);
        const auto value  = preset(name);
        const auto easing = value.easing();
        QCOMPARE(value.duration(), duration);
        QCOMPARE(easing.type(), QEasingCurve::BezierSpline);
        QCOMPARE(easing.toCubicSpline(), QList<QPointF>({ first, second, QPointF(1, 1) }));
        QCOMPARE(easing.valueForProgress(0), 0.0);
        QCOMPARE(easing.valueForProgress(1), 1.0);
        qreal maximum  = 0;
        qreal previous = 0;
        for (int i = 0; i <= 1000; ++i) {
            const auto sample = easing.valueForProgress(i / 1000.0);
            QVERIFY(sample >= 0);
            maximum = std::max(maximum, sample);
            if (! spatial) {
                QVERIFY(sample <= 1);
                QVERIFY(sample >= previous);
            }
            previous = sample;
        }
        QCOMPARE(maximum > 1.001, spatial);
        for (const auto* key : { "duration", "easing" }) {
            const auto property = token::MotionPreset::staticMetaObject.property(
                token::MotionPreset::staticMetaObject.indexOfProperty(key));
            QVERIFY(property.isConstant());
            QVERIFY(! property.isWritable());
        }
    }
    void qmlParameters_data() { parameters_data(); }
    void qmlParameters() {
        QFETCH(QString, name);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property string presetName
                property real value: 20
                readonly property var chosen: MD.Token.motion.expressive[presetName]
                function play() { motion.start() }
                NumberAnimation {
                    id: motion; objectName: "motion"
                    target: root; property: "value"; from: 20; to: 80
                    duration: root.chosen.duration
                    easing: root.chosen.easing
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "presetName", name } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* animation = object->findChild<QQuickNumberAnimation*>("motion");
        QVERIFY(animation);
        const auto expected = preset(name);
        QCOMPARE(animation->duration(), expected.duration());
        QCOMPARE(animation->easing(), expected.easing());
        QSignalSpy finished(animation, &QQuickAbstractAnimation::finished);
        QVERIFY(QMetaObject::invokeMethod(object.get(), "play"));
        QVERIFY(animation->isRunning());
        const int middle = animation->duration() / 2;
        animation->setCurrentTime(middle);
        const qreal expectedValue =
            20 + 60 * expected.easing().valueForProgress(qreal(middle) / expected.duration());
        QVERIFY(qAbs(object->property("value").toReal() - expectedValue) < .001);
        animation->complete();
        QCOMPARE(object->property("value").toReal(), 80.0);
        QVERIFY(! animation->isRunning());
        QCOMPARE(finished.size(), 1);
        animation->complete();
        QCOMPARE(finished.size(), 1);
    }
    void effects() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property color ink: "#ff0000"
                property real amount: 0.2
                ColorAnimation {
                    objectName: "color"; target: root; property: "ink"
                    from: "#ff0000"; to: "#0000ff"
                    duration: MD.Token.motion.expressive.defaultEffects.duration
                    easing: MD.Token.motion.expressive.defaultEffects.easing
                }
                NumberAnimation {
                    objectName: "opacity"; target: root; property: "amount"
                    from: 0.2; to: 0.8
                    duration: MD.Token.motion.expressive.defaultEffects.duration
                    easing: MD.Token.motion.expressive.defaultEffects.easing
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* color   = object->findChild<QQuickColorAnimation*>("color");
        auto* opacity = object->findChild<QQuickNumberAnimation*>("opacity");
        QVERIFY(color);
        QVERIFY(opacity);
        const auto expected = token::ExpressiveMotion {}.defaultEffects();
        QCOMPARE(color->easing(), expected.easing());
        QCOMPARE(opacity->easing(), expected.easing());
        color->start();
        opacity->start();
        color->setCurrentTime(100);
        opacity->setCurrentTime(100);
        const auto fraction = expected.easing().valueForProgress(.5);
        const auto ink      = object->property("ink").value<QColor>();
        QVERIFY(qAbs(ink.redF() - (1 - fraction)) < .01);
        QVERIFY(qAbs(ink.blueF() - fraction) < .01);
        QVERIFY(qAbs(object->property("amount").toReal() - (.2 + .6 * fraction)) < .001);
        color->complete();
        opacity->complete();
        QCOMPARE(object->property("ink").value<QColor>(), QColor(Qt::blue));
        QCOMPARE(object->property("amount").toReal(), .8);
    }
    void behaviorAndOverrides() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property real targetValue: 0
                property bool animate: true
                property int duration: MD.Token.motion.expressive.fastSpatial.duration
                property var curve: MD.Token.motion.expressive.fastSpatial.easing
                property real value: targetValue
                Behavior on value {
                    enabled: animate
                    NumberAnimation {
                        objectName: "motion"
                        duration: root.duration
                        easing: root.curve
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "targetValue", .6 } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("value").toReal(), .6);
        QVERIFY(object->setProperty("targetValue", 1.0));
        auto* animation = object->findChild<QQuickNumberAnimation*>("motion");
        QVERIFY(animation);
        QVERIFY(animation->isRunning());
        QTRY_VERIFY(object->property("value").toReal() != .6);
        const auto middle = object->property("value").toReal();
        QVERIFY(object->setProperty("targetValue", 0.0));
        QVERIFY(qAbs(object->property("value").toReal() - middle) < .0001);
        QTRY_COMPARE(object->property("value").toReal(), 0.0);
        QVERIFY(object->setProperty("duration", 0));
        QVERIFY(object->setProperty("targetValue", .8));
        QCOMPARE(object->property("value").toReal(), .8);
        QVERIFY(! animation->isRunning());
        QVERIFY(object->setProperty("duration", 700));
        const QEasingCurve custom(QEasingCurve::Linear);
        QVERIFY(object->setProperty("curve", QVariant::fromValue(custom)));
        QVERIFY(object->setProperty("targetValue", .4));
        QCOMPARE(animation->duration(), 700);
        QCOMPARE(animation->easing(), custom);
        QVERIFY(animation->isRunning());
        QVERIFY(object->setProperty("animate", false));
        QVERIFY(object->setProperty("targetValue", .2));
        QCOMPARE(object->property("value").toReal(), .2);
        QVERIFY(! animation->isRunning());
    }
    void transitionOverride() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                property real value: 0
                Behavior on value {
                    NumberAnimation {
                        objectName: "behavior"
                        duration: MD.Token.motion.expressive.slowSpatial.duration
                        easing: MD.Token.motion.expressive.slowSpatial.easing
                    }
                }
                states: State {
                    name: "active"
                    PropertyChanges { target: root; value: 100 }
                }
                transitions: Transition {
                    NumberAnimation {
                        objectName: "custom"; properties: "value"
                        duration: 120; easing.type: Easing.OutCubic
                    }
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QVERIFY(object->setProperty("state", "active"));
        auto* custom   = object->findChild<QQuickNumberAnimation*>("custom");
        auto* behavior = object->findChild<QQuickNumberAnimation*>("behavior");
        QVERIFY(custom);
        QCOMPARE(custom->duration(), 120);
        QCOMPARE(custom->easing(), QEasingCurve(QEasingCurve::OutCubic));
        QVERIFY(! behavior || ! behavior->isRunning());
        QTRY_COMPARE(object->property("value").toReal(), 100.0);
    }
    void example_data() {
        QTest::addColumn<int>("width");
        QTest::newRow("narrow") << 320;
        QTest::newRow("wide") << 900;
    }
    void example() {
        QFETCH(int, width);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        const auto path = QFINDTESTDATA("../../example/compositions/Motion.qml");
        QVERIFY(! path.isEmpty());
        QQmlComponent            component(&engine, QUrl::fromLocalFile(path));
        std::unique_ptr<QObject> object(
            component.createWithInitialProperties({ { "width", width }, { "height", 640 } }));
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        QVERIFY(root);
        QQuickWindow window;
        root->setParentItem(window.contentItem());
        polish(root);
        QCoreApplication::processEvents();
        polish(root);
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QVERIFY(! root->property("running").toBool());
        auto* controls = root->findChild<QQuickItem*>("motionControls");
        QVERIFY(controls);
        for (auto* child : controls->childItems()) {
            QVERIFY(child->x() >= 0);
            QVERIFY(child->x() + child->width() <= controls->width() + .1);
            QVERIFY(child->y() + child->height() <= controls->height() + .1);
        }
        auto* animation = root->findChild<QQuickNumberAnimation*>("spatialAnimation");
        QVERIFY(animation);
        QVERIFY(! root->findChild<QQuickColorAnimation*>("colorAnimation"));
        QVERIFY(! root->findChild<QQuickNumberAnimation*>("opacityAnimation"));
        QSignalSpy finished(animation, &QQuickAbstractAnimation::finished);
        auto*      play = root->findChild<QObject*>("playButton");
        QVERIFY(play);
        QVERIFY(QMetaObject::invokeMethod(play, "click"));
        animation->setCurrentTime(100);
        const auto middle = root->property("progress").toReal();
        QVERIFY(middle > 0);
        QVERIFY(root->setProperty("presetIndex", 1));
        QVERIFY(qAbs(root->property("progress").toReal() - middle) < .0001);
        QCOMPARE(animation->duration(), 500);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        QVERIFY(qAbs(root->property("progress").toReal() - middle) < .0001);
        QCOMPARE(finished.size(), 0);
        QVERIFY(QMetaObject::invokeMethod(root, "finish"));
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QCOMPARE(finished.size(), 1);
        QVERIFY(root->setProperty("presetIndex", 2));
        QCOMPARE(animation->duration(), 650);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        animation->setCurrentTime(200);
        QVERIFY(root->setProperty("animationsEnabled", false));
        QCOMPARE(root->property("progress").toReal(), 1.0);
        QVERIFY(! root->property("running").toBool());
        QVERIFY(QMetaObject::invokeMethod(root, "reset"));
        QCOMPARE(root->property("progress").toReal(), 0.0);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        QCOMPARE(root->property("progress").toReal(), 1.0);
        QVERIFY(! root->property("running").toBool());
        QVERIFY(root->setProperty("animationsEnabled", true));
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        root->setVisible(false);
        QVERIFY(! root->property("running").toBool());
        root->setVisible(true);
        QVERIFY(QMetaObject::invokeMethod(root, "toggleTarget"));
        QPointer<QQuickAbstractAnimation> guard(animation);
        const int                         before = finished.size();
        object.reset();
        QVERIFY(! guard);
        QCoreApplication::processEvents();
        QCOMPARE(finished.size(), before);
    }
};

int run_motion_presets(int argc, char** argv) {
    MotionPresetsTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "motion_presets.moc"
