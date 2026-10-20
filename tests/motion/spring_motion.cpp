#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QDir>
#include <cmath>
#include "qml_material/anim/spring_motion.hpp"
#include "qml_material/util/qml_util.hpp"

using namespace qml_material;

namespace
{
QAbstractAnimation* clock(QObject* motion) { return motion->findChild<QAbstractAnimation*>(); }

void finish(QObject* motion) {
    auto* animation = clock(motion);
    animation->setCurrentTime(animation->duration());
}
} // namespace

class SpringMotionTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|TypeError|ReferenceError|Cannot assign).*"));
    }

    void samples_data() {
        QTest::addColumn<qreal>("damping");
        for (qreal damping : { .6, .9, 1., 1.000001, 2. })
            QTest::newRow(qPrintable(QString::number(damping, 'g', 8))) << damping;
    }

    void samples() {
        QFETCH(qreal, damping);
        const token::SpringSpec spec { 1400, damping };
        const auto              initial = anim::sampleSpring(spec, 3, 11, -4, 0);
        QVERIFY(std::abs(initial.value - 3) < 1e-9);
        QVERIFY(std::abs(initial.velocity + 4) < 1e-9);
        for (qreal t : { .016, .1, .5, 2. }) {
            const auto  result = anim::sampleSpring(spec, 3, 11, -4, t);
            const qreal delta  = 1e-6;
            const auto  before = anim::sampleSpring(spec, 3, 11, -4, t - delta);
            const auto  after  = anim::sampleSpring(spec, 3, 11, -4, t + delta);
            QVERIFY(std::abs((after.value - before.value) / (2 * delta) - result.velocity) < 1e-5);
            const auto split   = anim::sampleSpring(spec, 3, 11, -4, t / 2);
            const auto resumed = anim::sampleSpring(spec, split.value, 11, split.velocity, t / 2);
            QVERIFY(std::abs(resumed.value - result.value) < 1e-8);
            QVERIFY(std::abs(resumed.velocity - result.velocity) < 1e-7);
        }
        const auto duration = anim::springDuration(spec, 3, 11, -4, .01);
        QVERIFY(duration > 0);
        const auto end = anim::sampleSpring(spec, 3, 11, -4, duration / 1000.0);
        QVERIFY(std::abs(end.value - 11) <= .01001);
        QCOMPARE(anim::springDuration(spec, 4, 4, 0, .01), 0);
        QVERIFY(anim::springDuration(spec, 4, 4, 200, .01) > 0);
    }

    void androidSamples() {
        // SpringSimulation: mass 1, critically damped, natural frequency 40 rad/s.
        const auto sample = anim::sampleSpring({ 1600, 1 }, 0, 1, 0, .1);
        QVERIFY(std::abs(sample.value - .9084218055563291) < 1e-12);
        QVERIFY(std::abs(sample.velocity - 2.9305022221974686) < 1e-12);
        const auto under = anim::sampleSpring({ 100, .5 }, 0, 1, 0, .1);
        QVERIFY(std::abs(under.value - .3402998466082983) < 1e-12);
        QVERIFY(std::abs(under.velocity - 5.33507195114693) < 1e-12);
    }

    void frameCadence() {
        SpringMotion fast, slow;
        fast.setTargetValue(10);
        slow.setTargetValue(10);
        for (int time = 8; time <= 96; time += 8) clock(&fast)->setCurrentTime(time);
        for (int time = 16; time <= 96; time += 16) clock(&slow)->setCurrentTime(time);
        QCOMPARE(fast.value(), slow.value());
        QCOMPARE(fast.velocity(), slow.velocity());
    }

    void retargetAndCompletion() {
        SpringMotion motion;
        motion.setSpec(token::SpringScheme {}.fastSpatial());
        QSignalSpy finished(&motion, &SpringMotion::finished);
        motion.setTargetValue(10);
        auto* animation = clock(&motion);
        QVERIFY(motion.isRunning());
        animation->setCurrentTime(30);
        const auto value = motion.value(), velocity = motion.velocity();
        QVERIFY(value > 0 && value < 10);
        motion.setTargetValue(0);
        QCOMPARE(motion.value(), value);
        QCOMPARE(motion.velocity(), velocity);
        QCOMPARE(animation->currentTime(), 0);
        motion.setTargetValue(20);
        QCOMPARE(motion.value(), value);
        QCOMPARE(motion.velocity(), velocity);
        animation->setCurrentTime(20);
        const auto time = animation->currentTime();
        motion.setTargetValue(20);
        QCOMPARE(animation->currentTime(), time);
        motion.setSpec(token::SpringScheme { true }.fastSpatial());
        QCOMPARE(animation->currentTime(), 0);
        finish(&motion);
        QCOMPARE(motion.value(), 20);
        QCOMPARE(motion.velocity(), 0);
        QVERIFY(! motion.isRunning());
        QCOMPARE(finished.size(), 1);
        motion.complete();
        QCOMPARE(finished.size(), 1);
        motion.setTargetValue(0);
        motion.stop();
        QVERIFY(! motion.isRunning());
        QCOMPARE(motion.value(), 20);
        QCOMPARE(motion.velocity(), 0);
        QCOMPARE(finished.size(), 1);
        motion.setTargetValue(5);
        motion.setEnabled(false);
        QCOMPARE(motion.value(), 5);
        QVERIFY(! motion.isRunning());
        QCOMPARE(finished.size(), 1);
        motion.snapTo(9);
        QCOMPARE(motion.value(), 9);
        QCOMPARE(motion.targetValue(), 9);
    }

    void invalidInput() {
        SpringMotion motion;
        motion.setTargetValue(10);
        clock(&motion)->setCurrentTime(20);
        const auto value = motion.value(), velocity = motion.velocity(),
                   spec = motion.spec().stiffness;
        motion.setTargetValue(std::numeric_limits<qreal>::quiet_NaN());
        motion.setSpec({ -1, 1 });
        motion.setSpec({ 1400, 0 });
        motion.setVisibilityThreshold(0);
        QCOMPARE(motion.targetValue(), 10);
        QCOMPARE(motion.value(), value);
        QCOMPARE(motion.velocity(), velocity);
        QCOMPARE(motion.spec().stiffness, spec);
        QCOMPARE(motion.visibilityThreshold(), .01);
    }

    void reentryAndDestruction() {
        SpringMotion motion;
        bool         reversed = false;
        connect(&motion, &SpringMotion::valueChanged, &motion, [&] {
            if (! reversed && motion.value() > 1) {
                reversed = true;
                motion.setTargetValue(-10);
            }
        });
        motion.setTargetValue(10);
        clock(&motion)->setCurrentTime(30);
        QVERIFY(reversed);
        QVERIFY(motion.isRunning());
        QCOMPARE(clock(&motion)->state(), QAbstractAnimation::Running);
        finish(&motion);
        QCOMPARE(motion.value(), -10);
        QPointer<SpringMotion> destroyed = new SpringMotion;
        connect(destroyed, &SpringMotion::valueChanged, &motion, [&] {
            delete destroyed.data();
        });
        destroyed->setTargetValue(10);
        clock(destroyed)->setCurrentTime(20);
        QVERIFY(! destroyed);
    }

    void idempotentCommands() {
        SpringMotion stopping;
        QSignalSpy   state(&stopping, &SpringMotion::runningChanged);
        connect(&stopping, &SpringMotion::valueChanged, &stopping, [&] {
            if (stopping.value() > 1) stopping.stop();
        });
        stopping.setTargetValue(10);
        clock(&stopping)->setCurrentTime(30);
        QVERIFY(! stopping.isRunning());
        QCOMPARE(stopping.velocity(), 0);
        QCOMPARE(state.size(), 2);
        stopping.stop();
        QCOMPARE(state.size(), 2);
        SpringMotion completing;
        QSignalSpy   done(&completing, &SpringMotion::finished);
        QSignalSpy   running(&completing, &SpringMotion::runningChanged);
        connect(&completing, &SpringMotion::valueChanged, &completing, [&] {
            if (completing.value() == 10) completing.complete();
        });
        completing.setTargetValue(10);
        finish(&completing);
        QCOMPARE(done.size(), 1);
        QCOMPARE(running.size(), 2);
        completing.complete();
        QCOMPARE(done.size(), 1);
    }

    void completionReentry() {
        SpringMotion motion;
        int          completed = 0;
        connect(&motion, &SpringMotion::finished, &motion, [&] {
            if (++completed == 1) motion.setTargetValue(-5);
        });
        motion.setTargetValue(10);
        finish(&motion);
        QCOMPARE(completed, 1);
        QVERIFY(motion.isRunning());
        QCOMPARE(clock(&motion)->state(), QAbstractAnimation::Running);
        finish(&motion);
        QCOMPARE(motion.value(), -5);
        QCOMPARE(completed, 2);
        QVERIFY(! motion.isRunning());
    }

    void springTokens_data() {
        QTest::addColumn<QString>("scheme");
        QTest::addColumn<QString>("preset");
        QTest::addColumn<qreal>("stiffness");
        QTest::addColumn<qreal>("damping");
        const QStringList  names { "fastSpatial", "defaultSpatial", "slowSpatial",
                                   "fastEffects", "defaultEffects", "slowEffects" };
        const QList<qreal> standard { 1400, 700, 300, 3800, 1600, 800 };
        const QList<qreal> expressive { 800, 380, 200, 3800, 1600, 800 };
        for (int i = 0; i < names.size(); ++i) {
            QTest::newRow(qPrintable("standard-" + names[i]))
                << QString("standard") << names[i] << standard[i] << (i < 3 ? .9 : 1.);
            QTest::newRow(qPrintable("expressive-" + names[i]))
                << QString("expressive") << names[i] << expressive[i]
                << (i == 0  ? .6
                    : i < 3 ? .8
                            : 1.);
        }
    }

    void springTokens() {
        QFETCH(QString, scheme);
        QFETCH(QString, preset);
        QFETCH(qreal, stiffness);
        QFETCH(qreal, damping);
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(
            QString(
                "import Qcm.Material as MD\nMD.SpringMotion { spec: MD.Token.motion.spring.%1.%2 }")
                .arg(scheme, preset)
                .toUtf8(),
            QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        const auto spec = qobject_cast<SpringMotion*>(object.get())->spec();
        QCOMPARE(spec.stiffness, stiffness);
        QCOMPARE(spec.dampingRatio, damping);
    }

    void quickRelease() {
        PressMotion motion;
        motion.setSpec(token::SpringScheme {}.fastSpatial());
        motion.setPressed(true);
        auto* animation = clock(&motion);
        animation->setCurrentTime(20);
        QVERIFY(motion.value() < .75);
        motion.setPressed(false);
        QCOMPARE(motion.targetValue(), 1);
        animation->setCurrentTime(100);
        QCOMPARE(motion.targetValue(), 0);
        QVERIFY(motion.value() > .75 && motion.value() < 1);
        QVERIFY(motion.isRunning());
        QCOMPARE(animation->state(), QAbstractAnimation::Running);
        finish(&motion);
        QCOMPARE(motion.value(), 0);
        motion.setActive(true);
        motion.setPressed(false);
        motion.setSpec({ 1, .1 });
        motion.setPressed(true);
        motion.setPressed(false);
        clock(&motion)->setCurrentTime(1000);
        QCOMPARE(motion.targetValue(), 0);
        motion.setActive(false);
        QVERIFY(! motion.isRunning());
        motion.setPressed(true);
        motion.setActive(false);
        QVERIFY(! motion.isRunning());
        QCOMPARE(motion.value(), 0);
    }

    void qmlInitializationAndTokens() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import Qcm.Material as MD
            MD.SpringMotion {
                targetValue: 42
                spec: MD.Token.motion.spring.expressive.fastSpatial
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* motion = qobject_cast<SpringMotion*>(object.get());
        QVERIFY(motion);
        QCOMPARE(motion->value(), 42);
        QVERIFY(! motion->isRunning());
        QCOMPARE(motion->spec(), (token::SpringSpec { 800, .6 }));
        QCOMPARE(token::SpringScheme {}.defaultEffects(), (token::SpringSpec { 1600, 1 }));
        QCOMPARE(token::SpringScheme { true }.slowSpatial(), (token::SpringSpec { 200, .8 }));
    }

    void colors() {
        ColorSpringMotion motion;
        motion.classBegin();
        motion.setTargetColor(Qt::red);
        motion.componentComplete();
        QCOMPARE(motion.color(), QColor(Qt::red));
        QVERIFY(! motion.isRunning());
        QCOMPARE(motion.spec(), (token::SpringSpec { 1600, 1 }));
        motion.setTargetColor(Qt::blue);
        clock(&motion)->setCurrentTime(40);
        const auto fraction = anim::sampleSpring(motion.spec(), 0, 1, 0, .04).value;
        QCOMPARE(motion.color(), Util::mixColorOklab(Qt::red, Qt::blue, fraction));
        const auto middle = motion.color();
        motion.setTargetColor(Qt::green);
        QCOMPARE(motion.color(), middle);
        motion.setTargetColor(Qt::red);
        QCOMPARE(motion.color(), middle);
        finish(&motion);
        QCOMPARE(motion.color(), QColor(Qt::red));
        motion.setTargetColor(QColor::fromRgbF(.2, .4, .6, .3));
        clock(&motion)->setCurrentTime(40);
        QVERIFY(motion.color().alphaF() > .3 && motion.color().alphaF() < 1);
        motion.setEnabled(false);
        QCOMPARE(motion.color(), motion.targetColor());
        QVERIFY(! motion.isRunning());
        const auto target = motion.targetColor();
        motion.setTargetColor({});
        QCOMPARE(motion.targetColor(), target);
        QVERIFY(! motion.isRunning());
    }
};

int run_spring_motion(int argc, char** argv) {
    SpringMotionTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "spring_motion.moc"
