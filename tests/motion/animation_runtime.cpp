#include "qml_material/anim/animatable.hpp"
#include "qml_material/anim/seekable_transition.hpp"
#include "qml_material/style/theme.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtTest>
#include <limits>
using namespace qml_material;
class AnimationRuntimeTest : public QObject {
    Q_OBJECT
private slots:
    void interruptionPauseAndOwnership() {
        Animatable a;
        a.setManualClock(true);
        QSignalSpy ended(&a, &Animatable::ended);
        const auto first = a.animateTo(100, Motion::tween(1000));
        a.advance(300);
        QCOMPARE(a.value(), 30.0);
        const auto v = a.velocity();
        a.pause();
        a.advance(700);
        QVERIFY(a.active());
        QVERIFY(a.paused());
        QVERIFY(! a.running());
        QCOMPARE(a.value(), 30.0);
        QCOMPARE(a.velocity(), v);
        a.resume();
        a.advance(100);
        QCOMPARE(a.value(), 40.0);
        const auto second = a.animateTo(0, Motion::spring());
        QCOMPARE(a.value(), 40.0);
        QCOMPARE(a.velocity(), v);
        a.stop(first);
        QVERIFY(a.running());
        a.advance(20);
        const auto held        = a.value();
        const auto interaction = a.beginInteraction();
        QCOMPARE(a.value(), held);
        QVERIFY(a.animateTo(7).isEmpty());
        QVERIFY(a.updateInteraction(interaction, 60));
        QVERIFY(! a.updateInteraction(first, 5));
        const auto released = a.settleInteraction(interaction, 0, -500, Motion::spring());
        QVERIFY(! released.isEmpty());
        QCOMPARE(a.value(), 60.0);
        QCOMPARE(a.velocity(), -500.0);
        QVERIFY(! a.cancelInteraction(interaction));
        a.complete();
        QVERIFY(! a.active());
        QCOMPARE(a.value(), 0.0);
        QTRY_COMPARE(ended.count(), 3);
        QCOMPARE(ended[0][0].toString(), first);
        QCOMPARE(ended[0][1].toInt(), int(Animatable::Interrupted));
        QCOMPARE(ended[1][0].toString(), second);
        QCOMPARE(ended[2][0].toString(), released);
    }
    void snapBoundsAndInvalid() {
        Animatable a;
        a.setManualClock(true);
        QSignalSpy ended(&a, &Animatable::ended);
        const auto id = a.animateTo(2, Motion::snap());
        QVERIFY(! id.isEmpty());
        QCOMPARE(a.value(), 2.0);
        QCOMPARE(ended.count(), 0);
        QTRY_COMPARE(ended.count(), 1);
        QVERIFY(a.setBounds(-10, 10));
        a.animateDecay(100, Motion::decay());
        a.advance(1000);
        QCOMPARE(a.value(), 10.0);
        QVERIFY(! a.active());
        QTRY_COMPARE(ended.count(), 2);
        QCOMPARE(ended[1][1].toInt(), int(Animatable::BoundReached));
        a.animateTo(-5, Motion::tween(1000));
        a.advance(100);
        const auto value = a.value(), velocity = a.velocity();
        QVERIFY(a.animateTo(std::numeric_limits<double>::quiet_NaN()).isEmpty());
        auto invalid = Motion::spring(-1);
        QVERIFY(a.animateTo(0, invalid).isEmpty());
        QCOMPARE(a.value(), value);
        QCOMPARE(a.velocity(), velocity);
        a.stop();
        QCOMPARE(a.value(), value);
        QCOMPARE(a.velocity(), 0.0);
    }
    void preparedCadenceAndVelocity() {
        MotionRunner fast(2), slow(2);
        fast.setManualClock(true);
        slow.setManualClock(true);
        fast.animateTo({ 100, 0 }, Motion::spring(), anim::Vector { 10, 200 });
        slow.animateTo({ 100, 0 }, Motion::spring(), anim::Vector { 10, 200 });
        for (int i = 0; i < 12; ++i) fast.advance(1000.0 / 120);
        for (int i = 0; i < 6; ++i) slow.advance(1000.0 / 60);
        for (int i = 0; i < 2; ++i) {
            QVERIFY(std::abs(fast.sample().value[i] - slow.sample().value[i]) < 1e-9);
            QVERIFY(std::abs(fast.sample().velocity[i] - slow.sample().velocity[i]) < 1e-8);
        }
        QVERIFY(fast.sample().value[1] != 0);
        auto* runner = new MotionRunner(1);
        connect(runner, &MotionRunner::changed, runner, [runner] {
            delete runner;
        });
        runner->animateTo({ 1 }, Motion::snap());
    }
    void themeLifecycle() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent c(&engine);
        c.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    id: root
    MD.MProp.motion.durationScale: 2
    MD.MProp.motion.container: MD.Motion.tween(900)
    Item { id: other; objectName: "other"; MD.MProp.motion.durationScale: 3 }
    MD.Animatable { objectName: "value"; manualClock: true }
    MD.Popup {
        objectName: "popup"; parent: root
        contentItem: Item {
            MD.Animatable { objectName: "popupValue"; manualClock: true }
        }
    }
})",
                  QUrl());
        std::unique_ptr<QObject> root(c.create());
        QVERIFY2(root, qPrintable(c.errorString()));
        auto* value      = root->findChild<Animatable*>("value");
        auto* other      = root->findChild<QObject*>("other");
        auto* popupValue = root->findChild<Animatable*>("popupValue");
        QCOMPARE(popupValue->settings()->durationScale(), 2.0);
        QCOMPARE(popupValue->settings()->container().duration, 900);
        value->setParent(other);
        QTRY_COMPARE(value->settings()->durationScale(), 3.0);
        value->setParent(root.get());
        QTRY_COMPARE(value->settings()->durationScale(), 2.0);
        MotionSettings settings;
        value->setSettings(&settings);
        value->animateTo(10, Motion::tween(1000));
        value->advance(100);
        value->pause();
        settings.setSpatial(Motion::spring(200, .6));
        QVERIFY(value->paused());
        QCOMPARE(value->value(), 1.0);
        settings.setDurationScale(0);
        QCOMPARE(value->value(), 10.0);
        QVERIFY(! value->active());
        const auto id = value->beginInteraction();
        value->updateInteraction(id, 3);
        QCOMPARE(value->value(), 3.0);
        value->settleInteraction(id, 7, 0, Motion::spring());
        QCOMPARE(value->value(), 7.0);
        auto* temporary = new MotionSettings;
        temporary->setDurationScale(5);
        value->setSettings(temporary);
        delete temporary;
        QCOMPARE(value->settings(), nullptr);
        QVERIFY(! value->animateTo(0).isEmpty());
    }
    void qmlTypesAndTheme() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
import QtQuick
import Qcm.Material as MD
Item {
    MD.MProp.motion.durationScale: 2
    MD.MProp.motion.spatial: MD.Motion.spring(900, 1)
    MD.Animatable { objectName: "number"; initialValue: 7; manualClock: true; spec: MD.Motion.tween(100) }
    MD.PointAnimatable { initialValue: Qt.point(2, 3) }
    MD.SizeAnimatable { initialValue: Qt.size(4, 5) }
    MD.ColorAnimatable { initialValue: "red" }
    MD.AnimatedReal { objectName: "follow"; targetValue: 9 }
    Item {
        objectName: "child"
        MD.MProp.motion.reducedMotion: true
    }
    MD.SeekableTransition {
        initialState: "closed"
        MD.RealTrack { values: ({closed: 0, open: 1}); spec: MD.Motion.tween(200) }
        MD.PointTrack { values: ({closed: Qt.point(0, 0), open: Qt.point(20, 10)}); spec: MD.Motion.spring() }
    }
})",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* a = root->findChild<Animatable*>("number");
        QVERIFY(a);
        QCOMPARE(a->value(), 7.0);
        a->animateTo(17);
        a->advance(100);
        QCOMPARE(a->value(), 12.0);
        auto* child = root->findChild<QObject*>("child");
        auto* theme = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(child, true));
        QVERIFY(theme->motion()->reducedMotion());
        QCOMPARE(theme->motion()->durationScale(), 2.0);
        QCOMPARE(theme->motion()->spatial().stiffness, 900.0);
        theme->motion()->setDurationScale(3);
        theme->motion()->resetDurationScale();
        QCOMPARE(theme->motion()->durationScale(), 2.0);
    }
};
int run_animation_runtime(int argc, char** argv) {
    AnimationRuntimeTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "animation_runtime.moc"
