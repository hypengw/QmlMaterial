#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QtQuick/private/qquickpath_p.h>
#include <QLineF>
#include <cmath>
#include <limits>
#include "qml_material/control/progress_indicator.hpp"

using namespace qml_material;

template<typename T>
class SampledIndicator : public T {
public:
    using ProgressIndicator::advanceAnimations;
    using ProgressIndicator::componentComplete;
    using T::T;
};

class ProgressIndicatorTest : public QObject {
    Q_OBJECT
private:
    static bool finite(const QPainterPath& path) {
        for (int i = 0; i < path.elementCount(); ++i)
            if (! std::isfinite(path.elementAt(i).x) || ! std::isfinite(path.elementAt(i).y))
                return false;
        return true;
    }
private slots:
    void measurement() {
        MeasuredPath line({ Cubic::line({ 0, 0 }, { 100, 0 }) });
        QCOMPARE(line.length(), 100.0);
        QCOMPARE(line.location(37).point, QPointF(37, 0));
        QCOMPARE(line.location(37).tangent, QPointF(1, 0));
        const auto part = line.segment(20, 80);
        QVERIFY(QLineF(part.pointAtPercent(0), QPointF(20, 0)).length() < .002);
        QVERIFY(QLineF(part.pointAtPercent(1), QPointF(80, 0)).length() < .002);
        MeasuredPath nonlinear({ { { 0, 0 }, { 0, 0 }, { 100, 0 }, { 100, 0 } } });
        QVERIFY(QLineF(nonlinear.location(10).point, QPointF(10, 0)).length() < .003);
        const Cubic  c { { 0, 0 }, { 0, 100 }, { 100, 100 }, { 100, 0 } };
        MeasuredPath curved({ c });
        QVERIFY(std::abs(curved.length() - 200) < .003);
        QCOMPARE(curved.location(0).tangent, QPointF(0, 1));
        QVERIFY(QLineF(curved.location(curved.length() / 2).point, c.point(.5)).length() < .003);
        QVERIFY(
            QLineF(curved.segment(10, 180).pointAtPercent(1), curved.location(180).point).length() <
            .003);
        MeasuredPath empty;
        QCOMPARE(empty.length(), 0.0);
        QVERIFY(empty.segment(-1, 1).isEmpty());
        QVERIFY(curved.segment(0, std::numeric_limits<double>::quiet_NaN()).isEmpty());
        const auto   arc = Cubic::arc({}, { 20, 0 }, { 0, 20 });
        MeasuredPath quarter({ arc });
        QVERIFY(std::abs(quarter.length() - 10 * std::acos(-1.0)) < .01);
        std::vector<Cubic>           ring;
        const std::array<QPointF, 5> anchors = {
            QPointF(20, 0), QPointF(0, 20), QPointF(-20, 0), QPointF(0, -20), QPointF(20, 0)
        };
        for (int i = 0; i < 4; ++i) ring.push_back(Cubic::arc({}, anchors[i], anchors[i + 1]));
        MeasuredPath closed(ring);
        const auto   crossing = closed.cyclicSegment(closed.length() * .9, closed.length() * .3);
        QVERIFY(QLineF(crossing.pointAtPercent(0), closed.location(closed.length() * .9).point)
                    .length() < .003);
        QVERIFY(QLineF(crossing.pointAtPercent(1), closed.location(closed.length() * .2).point)
                    .length() < .003);
    }
    void geometry_data() {
        QTest::addColumn<bool>("circular");
        QTest::newRow("linear") << false;
        QTest::newRow("circular") << true;
    }
    void geometry() {
        QFETCH(bool, circular);
        ProgressIndicatorGeometry geometry;
        ProgressIndicatorState    state;
        state.circular      = circular;
        state.wavy          = true;
        state.waveLength    = circular ? 15 : 40;
        state.waveAmplitude = circular ? 1.6 : 3;
        state.trackColor    = Qt::gray;
        state.stopColor     = Qt::black;
        const auto size     = circular ? QSizeF(96, 48) : QSizeF(240, 10);
        for (const auto value : { 0., .005, .01, .1, .5, .9, .95, .99, 1. }) {
            state.position = value;
            state.segments = { { 0, value, Qt::red } };
            for (const auto phase : { 0., .25, .999999, 1. }) {
                state.phase      = phase;
                const auto paths = geometry.render(state, size);
                QVERIFY(! paths.empty());
                for (const auto& p : paths) QVERIFY(finite(p.path));
                if (circular) {
                    for (const auto& p : paths) {
                        const auto bounds = p.path.boundingRect();
                        QVERIFY(bounds.left() >= 24 - .01 && bounds.right() <= 72 + .01);
                        QVERIFY(bounds.top() >= -.01 && bounds.bottom() <= 48 + .01);
                    }
                }
            }
        }
        const auto revision = geometry.cacheRevision();
        state.phase         = .7;
        state.trackColor    = Qt::blue;
        geometry.render(state, size);
        QCOMPARE(geometry.cacheRevision(), revision);
        geometry.render(state, size * 2);
        QVERIFY(geometry.cacheRevision() > revision);
        QVERIFY(geometry.render(state, {}).empty());
        state.waveLength = 10000;
        const auto tiny  = geometry.render(state, QSizeF(2, 2));
        for (const auto& p : tiny) QVERIFY(finite(p.path));
        state.waveAmplitude = 0;
        QVERIFY(! geometry.render(state, size).empty());
    }
    void nativeProgress() {
        SampledIndicator<LinearIndicator> indicator;
        indicator.setIndeterminate(false);
        indicator.setWavy(true);
        indicator.setValue(.5);
        indicator.setCompletionBehavior(ProgressIndicator::Keep);
        indicator.componentComplete();
        QCOMPARE(indicator.displayedPosition(), .5);
        QCOMPARE(indicator.amplitudeFraction(), 1.0);
        QCOMPARE(indicator.waveLength(), 40.0);
        QCOMPARE(indicator.preferredSize(), QSizeF(240, 10));
        indicator.setValue(.9);
        indicator.advanceAnimations(75);
        const auto before = indicator.displayedPosition();
        QVERIFY(before > .5 && before < .9);
        indicator.setValue(.2);
        QCOMPARE(indicator.displayedPosition(), before);
        indicator.advanceAnimations(150);
        QCOMPARE(indicator.displayedPosition(), .2);
        indicator.setValue(1);
        indicator.advanceAnimations(1000);
        QCOMPARE(indicator.displayedPosition(), 1.0);
        QCOMPARE(indicator.renderState().segments.front().end, 1.0);
        QCOMPARE(indicator.amplitudeFraction(), 0.0);
        indicator.setValue(.5);
        indicator.advanceAnimations(500);
        QCOMPARE(indicator.renderState().segments.front().end, .5);
        indicator.setCompletionBehavior(ProgressIndicator::Drain);
        indicator.setValue(1);
        indicator.advanceAnimations(900);
        QCOMPARE(indicator.renderState().drain, 1.0);
        indicator.setValue(.5);
        indicator.advanceAnimations(900);
        QCOMPARE(indicator.renderState().drain, 0.0);
        indicator.setAnimationsEnabled(false);
        indicator.setValue(.7);
        QCOMPARE(indicator.displayedPosition(), .7);
        indicator.setValue(std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(indicator.value(), .7);
        indicator.setFrom(10);
        indicator.setTo(0);
        indicator.setValue(4);
        QCOMPARE(indicator.position(), .6);
        QCOMPARE(indicator.displayedPosition(), .6);
    }
    void amplitudeAndElapsed() {
        SampledIndicator<LinearIndicator> indicator;
        indicator.setWavy(true);
        indicator.setIndeterminate(false);
        indicator.setCompletionBehavior(ProgressIndicator::Keep);
        indicator.setValue(.1);
        indicator.componentComplete();
        QCOMPARE(indicator.amplitudeFraction(), 1.0);
        indicator.setValue(.9);
        indicator.advanceAnimations(500);
        QCOMPARE(indicator.amplitudeFraction(), 1.0);
        indicator.setValue(.95);
        indicator.advanceAnimations(250);
        QVERIFY(indicator.amplitudeFraction() > 0 && indicator.amplitudeFraction() < 1);
        indicator.advanceAnimations(250);
        QCOMPARE(indicator.amplitudeFraction(), 0.0);
        indicator.setValue(.5);
        indicator.advanceAnimations(500);
        indicator.advanceAnimations(1000);
        const auto phase = indicator.phase();
        indicator.advanceAnimations(123);
        QVERIFY(std::abs(std::fmod(indicator.phase() - phase + 1, 1) - .123) < .000001);
        indicator.advanceAnimations(877);
        QVERIFY(std::abs(indicator.phase() - phase) < .000001);
        indicator.setWaveCycleDuration(0);
        indicator.advanceAnimations(1000);
        QVERIFY(std::abs(indicator.phase() - phase) < .000001);
    }
    void phaseAndSeam() {
        ProgressIndicatorState s;
        s.wavy        = true;
        s.waveLength  = 40;
        s.trackColor  = Qt::gray;
        s.stopVisible = false;
        s.segments    = { { .2, .8, Qt::red } };
        ProgressIndicatorGeometry geometry;
        const auto                paths = geometry.render(s, { 240, 10 });
        QPainterPath              active;
        for (const auto& p : paths)
            if (p.color == QColor(Qt::red)) active = p.path;
        QVERIFY(! active.isEmpty());
        s.segments.front().start = .4;
        QPainterPath moved;
        for (const auto& p : geometry.render(s, { 240, 10 }))
            if (p.color == QColor(Qt::red)) moved = p.path;
        QVERIFY(QLineF(active.pointAtPercent(1), moved.pointAtPercent(1)).length() < .002);
        s.phase = 1;
        for (const auto& p : geometry.render(s, { 240, 10 }))
            if (p.color == QColor(Qt::red)) QCOMPARE(p.path, moved);
        s.phase = .5;
        for (const auto& p : geometry.render(s, { 240, 10 }))
            if (p.color == QColor(Qt::red)) QVERIFY(p.path != moved);
        s.circular = true;
        s.position = 1;
        s.segments = { { 0, 1, Qt::red } };
        for (const auto& p : geometry.render(s, { 48, 48 })) {
            if (p.color == QColor(Qt::red)) {
                QVERIFY(QLineF(p.path.pointAtPercent(0), p.path.pointAtPercent(1)).length() < .002);
                QVERIFY(p.path.elementCount() > 4);
            }
        }
        const auto revision = geometry.cacheRevision();
        s.amplitudeFraction = .5;
        geometry.render(s, { 48, 48 });
        QVERIFY(geometry.cacheRevision() > revision);
        s.circular = false;
        s.mirrored = true;
        s.position = .5;
        s.segments = { { 0, .5, Qt::red } };
        for (const auto& p : geometry.render(s, { 240, 10 }))
            if (p.color == QColor(Qt::red))
                QVERIFY(p.path.pointAtPercent(0).x() > p.path.pointAtPercent(1).x());
        s.rotation = std::numeric_limits<double>::quiet_NaN();
        QVERIFY(geometry.render(s, { 240, 10 }).empty());
        s.rotation   = 0;
        s.waveLength = -1;
        QVERIFY(geometry.render(s, { 240, 10 }).empty());
    }
    void notificationReentry() {
        auto* indicator = new SampledIndicator<LinearIndicator>;
        indicator->componentComplete();
        QPointer<ProgressIndicator> guard(indicator);
        connect(indicator, &ProgressIndicator::colorChanged, indicator, [indicator] {
            delete indicator;
        });
        indicator->setColor(Qt::red);
        QVERIFY(! guard);
        auto* frame = new SampledIndicator<CircularIndicator>;
        frame->componentComplete();
        QPointer<ProgressIndicator> frameGuard(frame);
        connect(frame, &ProgressIndicator::frameChanged, frame, [frame] {
            delete frame;
        });
        frame->advanceAnimations(10);
        QVERIFY(! frameGuard);
        auto* ranged = new SampledIndicator<LinearIndicator>;
        ranged->setIndeterminate(false);
        ranged->componentComplete();
        QPointer<ProgressIndicator> rangeGuard(ranged);
        connect(ranged, &ProgressIndicator::frameChanged, ranged, [ranged] {
            delete ranged;
        });
        ranged->setValue(.5);
        QVERIFY(! rangeGuard);
        SampledIndicator<LinearIndicator> reentrant;
        reentrant.componentComplete();
        connect(&reentrant, &ProgressIndicator::wavyChanged, &reentrant, [&reentrant] {
            reentrant.setWaveLength(70);
        });
        reentrant.setWavy(true);
        QCOMPARE(reentrant.waveLength(), 70.0);
    }
    void bridgeOwnership() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport Qcm.Material as MD\nMD.ProgressIndicatorShape { "
                          "width: 240; height: 10 }",
                          QUrl());
        std::unique_ptr<QObject> bridge(component.create());
        QVERIFY2(bridge, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(240, 20);
        window.show();
        qobject_cast<QQuickItem*>(bridge.get())->setParentItem(window.contentItem());
        auto source = std::make_unique<SampledIndicator<LinearIndicator>>();
        source->setWavy(true);
        source->setIndeterminate(false);
        source->setValue(.5);
        source->componentComplete();
        bridge->setProperty("source",
                            QVariant::fromValue(static_cast<ProgressIndicator*>(source.get())));
        QTRY_VERIFY(! bridge->findChildren<QQuickPath*>().empty());
        const auto pathObjects = bridge->findChildren<QQuickPath*>();
        auto       paths       = [&] {
            QList<QPainterPath> result;
            for (auto* p : pathObjects) result.push_back(p->path());
            return result;
        };
        const auto first = paths();
        source->advanceAnimations(250);
        QTRY_VERIFY(paths() != first);
        QCOMPARE(bridge->findChildren<QQuickPath*>(), pathObjects);
        auto second = std::make_unique<SampledIndicator<LinearIndicator>>();
        second->setIndeterminate(false);
        second->setValue(.3);
        second->componentComplete();
        bridge->setProperty("source",
                            QVariant::fromValue(static_cast<ProgressIndicator*>(second.get())));
        source.reset();
        QCOMPARE(bridge->property("source").value<ProgressIndicator*>(),
                 static_cast<ProgressIndicator*>(second.get()));
        second.reset();
        QVERIFY(! bridge->property("source").value<ProgressIndicator*>());
        QTRY_VERIFY(std::all_of(pathObjects.begin(), pathObjects.end(), [](auto* path) {
            return path->path().isEmpty();
        }));
    }
    void legacyHelperUpdates() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.LinearIndicatorWaveShape {
                width: 240; height: 10
                indicators: updater ? updater.activeIndicators : []
                MD.LinearIndicatorUpdator {
                    id: updater; objectName: "updater"; colors: ["red"]; progress: .75
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QPointF bounds;
        QVERIFY(QMetaObject::invokeMethod(root.get(),
                                          "drawLine",
                                          Q_RETURN_ARG(QPointF, bounds),
                                          Q_ARG(qreal, 0),
                                          Q_ARG(qreal, .5),
                                          Q_ARG(qreal, 2)));
        QCOMPARE(bounds, QPointF(0, 116));
        QQuickWindow window;
        window.resize(240, 20);
        window.show();
        qobject_cast<QQuickItem*>(root.get())->setParentItem(window.contentItem());
        auto paths = [&] {
            QList<QPainterPath> result;
            for (auto* path : root->findChildren<QQuickPath*>()) result.push_back(path->path());
            return result;
        };
        QTRY_VERIFY(! paths().empty());
        const auto first   = paths();
        auto*      updater = root->findChild<QObject*>("updater");
        QVERIFY(updater);
        updater->setProperty("progress", .8);
        QTRY_VERIFY(paths() != first);
        delete updater;
        const auto remaining = root->findChildren<QQuickPath*>();
        QTRY_VERIFY(std::all_of(remaining.begin(), remaining.end(), [](auto* path) {
            return path->path().isEmpty();
        }));
    }
    void defaultsAndOverrides() {
        SampledIndicator<CircularIndicator> circle;
        circle.setWavy(true);
        circle.componentComplete();
        QCOMPARE(circle.type(), 1);
        QCOMPARE(circle.preferredSize(), QSizeF(48, 48));
        circle.setType(0);
        circle.setWavy(false);
        circle.setWavy(true);
        QCOMPARE(circle.type(), 0);
        circle.resetType();
        QCOMPARE(circle.type(), 1);
        circle.advanceAnimations(100);
        circle.setRunning(false);
        QCOMPARE(circle.animationState(), ProgressIndicator::Completing);
        circle.setWavy(false);
        QCOMPARE(circle.animationState(), ProgressIndicator::Stopped);
        circle.advanceAnimations(1000);
        QCOMPARE(circle.animationState(), ProgressIndicator::Stopped);
        circle.setWavy(true);
        circle.setGapSize(7);
        circle.setGapAngle(10);
        circle.setGapSize(9);
        QCOMPARE(circle.renderState().gapAngle, 10.0);
        circle.resetGapAngle();
        QCOMPARE(circle.renderState().gapAngle, -1.0);
        QCOMPARE(circle.renderState().gapSize, 9.0);
        SampledIndicator<LinearIndicator> line;
        line.setWavy(true);
        QCOMPARE(line.waveLength(), 20.0);
        line.setWaveLength(70);
        line.setIndeterminate(false);
        QCOMPARE(line.waveLength(), 70.0);
        line.resetWaveLength();
        QCOMPARE(line.waveLength(), 40.0);
        line.setWaveCycleDuration(1800);
        line.setWavy(false);
        QCOMPARE(line.waveCycleDuration(), 1800);
        line.resetWaveCycleDuration();
        QCOMPARE(line.waveCycleDuration(), 1200);
        line.setWaveLength(-1);
        QCOMPARE(line.waveLength(), 30.0);
        line.setWaveAmplitude(std::numeric_limits<double>::infinity());
        QCOMPARE(line.waveAmplitude(), 3.0);
        line.setWaveLength(0);
        line.setWaveCycleDuration(0);
        line.componentComplete();
        line.advanceAnimations(1000);
        QCOMPARE(line.phase(), 0.0);
    }
    void timing_data() {
        QTest::addColumn<bool>("circular");
        QTest::addColumn<int>("type");
        QTest::addColumn<double>("cycle");
        QTest::newRow("disjoint") << false << 0 << 1800.;
        QTest::newRow("contiguous") << false << 1 << 333.;
        QTest::newRow("advance") << true << 0 << 5400.;
        QTest::newRow("retreat") << true << 1 << 6000.;
    }
    void timing() {
        QFETCH(bool, circular);
        QFETCH(int, type);
        QFETCH(double, cycle);
        SampledIndicator<LinearIndicator>   line;
        SampledIndicator<CircularIndicator> circle;
        ProgressIndicator* item = circular ? static_cast<ProgressIndicator*>(&circle) : &line;
        item->setWavy(true);
        item->setType(type);
        if (circular)
            circle.componentComplete();
        else
            line.componentComplete();
        auto advance = [&](double ms) {
            if (circular)
                circle.advanceAnimations(ms);
            else
                line.advanceAnimations(ms);
        };
        QCOMPARE(item->animationState(), ProgressIndicator::Running);
        advance(cycle * .25);
        QVERIFY(std::abs(item->progress() - .25) < .00001);
        const auto running = item->renderState();
        QVERIFY(! running.segments.empty());
        advance(cycle * .75);
        QVERIFY(std::abs(item->progress()) < .00001);
        advance(cycle * .5);
        item->setRunning(false);
        QCOMPARE(item->animationState(), ProgressIndicator::Completing);
        advance(circular ? (type ? 500 : 333) : cycle * .5);
        QCOMPARE(item->animationState(), ProgressIndicator::Stopped);
        advance(100);
        QCOMPARE(item->renderState().opacity, 0.0);
        item->setRunning(true);
        advance(100);
        item->setRunning(false);
        item->setRunning(true);
        advance(1000);
        QCOMPARE(item->animationState(), ProgressIndicator::Running);
        item->setValue(.7);
        item->setIndeterminate(false);
        advance(500);
        QCOMPARE(item->displayedPosition(), .7);
        QCOMPARE(item->renderState().segments.front().end, .7);
        item->setIndeterminate(true);
        item->setAnimationsEnabled(false);
        QCOMPARE(item->animationState(), ProgressIndicator::Running);
        QVERIFY(item->renderState().segments.front().end >
                item->renderState().segments.front().start);
        item->setRunning(false);
        QCOMPARE(item->animationState(), ProgressIndicator::Stopped);
    }
    void qmlCompatibilityAndLifecycle() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 320; height: 120
                MD.LinearIndicator { objectName: "line"; width: 240; wavy: true; type: MD.LinearIndicator.Contiguous }
                MD.CircularIndicator { objectName: "circle"; y: 40; wavy: true; type: MD.CircularIndicator.Reteat; completionBehavior: MD.CircularIndicator.Keep }
                MD.LinearIndicatorWaveShape { y: 100; width: 240; height: 10; indicators: [{startFraction: 0, endFraction: .5, color: "red", gapSize: 4}] }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* line   = root->findChild<LinearIndicator*>("line");
        auto* circle = root->findChild<CircularIndicator*>("circle");
        QVERIFY(line && circle);
        QCOMPARE(line->type(), 1);
        QCOMPARE(circle->type(), 1);
        QCOMPARE(circle->completionBehavior(), ProgressIndicator::Keep);
        QCOMPARE(line->implicitHeight(), 10.0);
        QCOMPARE(circle->implicitWidth(), 48.0);
        QQuickWindow window;
        window.resize(320, 120);
        auto* item = qobject_cast<QQuickItem*>(root.get());
        item->setParentItem(window.contentItem());
        window.show();
        QTRY_VERIFY(line->animating());
        QTRY_VERIFY(line->progress() > 0);
        item->setVisible(false);
        QVERIFY(! line->animating());
        const auto phase = line->phase();
        QCoreApplication::processEvents();
        QCOMPARE(line->phase(), phase);
        item->setVisible(true);
        QVERIFY(line->animating());
        line->setEnabled(false);
        QVERIFY(! line->animating());
        line->setEnabled(true);
        QVERIFY(line->animating());
        window.hide();
        QVERIFY(! line->animating());
        window.show();
        QTRY_VERIFY(line->animating());
        item->setParentItem(nullptr);
        QVERIFY(! line->animating());
        QQuickWindow other;
        other.resize(320, 120);
        other.show();
        item->setParentItem(other.contentItem());
        QTRY_VERIFY(line->animating());
        line->setAnimationsEnabled(false);
        QVERIFY(! line->animating());
        line->setAnimationsEnabled(true);
        QVERIFY(line->animating());
        line->setRunning(false);
        QTRY_COMPARE(line->animationState(), ProgressIndicator::Stopped);
        QTRY_VERIFY(! line->animating());
    }
    void initialViewportLifecycle() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQuickWindow window;
        window.resize(100, 100);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* viewport = new QQuickItem(window.contentItem());
        viewport->setSize({ 100, 40 });
        viewport->setClip(true);
        auto*                     line = new SampledIndicator<LinearIndicator>(viewport);
        QPointer<LinearIndicator> guard(line);
        line->setSize({ 80, 10 });
        line->setY(50);
        line->setWavy(true);
        line->componentComplete();
        QVERIFY(! line->animating());
        viewport->setHeight(80);
        QVERIFY(line->animating());
        QTRY_VERIFY(line->phase() > 0);
        viewport->setHeight(40);
        QVERIFY(! line->animating());
        const auto phase = line->phase();
        QSignalSpy windowFrames(&window, &QQuickWindow::afterAnimating);
        window.update();
        QTRY_VERIFY(! windowFrames.isEmpty());
        QCOMPARE(line->phase(), phase);
        delete viewport;
        QVERIFY(! guard);
        windowFrames.clear();
        window.update();
        QTRY_VERIFY(! windowFrames.isEmpty());
    }
    void viewportLifecycle_data() {
        QTest::addColumn<QString>("viewportType");
        QTest::newRow("qt-flickable") << QStringLiteral("MD.VerticalFlickable");
        QTest::newRow("native-scrollable") << QStringLiteral("MD.Scrollable");
    }
    void viewportLifecycle() {
        QFETCH(QString, viewportType);
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(QString(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 320; height: 320
                Item {
                    objectName: "outer"; width: 200; height: 110; clip: true
                    %1 {
                        objectName: "viewport"; width: 180; height: 100; clip: true
                        contentWidth: 180; contentHeight: 500
                        Item {
                            id: host
                            objectName: "host"; width: 180; height: 500
                            property real offset: 0
                            transform: Translate { x: host.offset }
                            MD.LinearIndicator {
                                objectName: "line"; y: 240; width: 140; wavy: true
                                indeterminate: false; value: .5
                                completionBehavior: MD.LinearIndicator.Keep
                            }
                            MD.CircularIndicator {
                                objectName: "circle"; y: 240; x: 140; wavy: true
                            }
                        }
                    }
                }
            }
        )")
                              .arg(viewportType)
                              .toUtf8(),
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* line     = root->findChild<LinearIndicator*>("line");
        auto* circle   = root->findChild<CircularIndicator*>("circle");
        auto* viewport = root->findChild<QQuickItem*>("viewport");
        auto* outer    = root->findChild<QQuickItem*>("outer");
        auto* host     = root->findChild<QQuickItem*>("host");
        auto* item     = qobject_cast<QQuickItem*>(root.get());
        QVERIFY(line && circle && viewport && outer && host && item);
        QQuickWindow window;
        window.resize(320, 320);
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(! line->animating());
        QVERIFY(! circle->animating());
        QVERIFY(line->isVisible() && circle->isVisible());
        QVERIFY(circle->running());
        QCOMPARE(circle->animationState(), ProgressIndicator::Running);

        viewport->setProperty("contentY", 240);
        QTRY_VERIFY(line->animating() && circle->animating());
        QTRY_VERIFY(line->phase() > 0);
        viewport->setProperty("contentY", 288);
        QTRY_VERIFY(! line->animating() && ! circle->animating());
        const auto linePhase      = line->phase();
        const auto circlePhase    = circle->phase();
        const auto circleProgress = circle->progress();
        QSignalSpy lineFrames(line, &ProgressIndicator::frameChanged);
        QSignalSpy circleFrames(circle, &ProgressIndicator::frameChanged);
        QSignalSpy windowFrames(&window, &QQuickWindow::afterAnimating);
        window.update();
        QTRY_VERIFY(! windowFrames.isEmpty());
        QCOMPARE(line->phase(), linePhase);
        QCOMPARE(circle->phase(), circlePhase);
        QCOMPARE(circle->progress(), circleProgress);
        QVERIFY(lineFrames.isEmpty() && circleFrames.isEmpty());

        viewport->setProperty("contentY", 270);
        QTRY_VERIFY(! line->animating() && circle->animating());
        viewport->setProperty("contentY", 240);
        QVERIFY(line->animating() && circle->animating());
        QCOMPARE(line->phase(), linePhase);
        viewport->setProperty("contentY", 0);
        QTRY_VERIFY(! line->animating() && ! circle->animating());
        viewport->setHeight(300);
        QVERIFY(! line->animating() && ! circle->animating());
        outer->setHeight(300);
        QTRY_VERIFY(line->animating() && circle->animating());
        viewport->setHeight(240);
        QVERIFY(! line->animating() && ! circle->animating());
        viewport->setHeight(241);
        QVERIFY(line->animating() && circle->animating());
        viewport->setHeight(100);
        QVERIFY(! line->animating() && ! circle->animating());
        viewport->setClip(false);
        QVERIFY(line->animating() && circle->animating());
        outer->setHeight(110);
        QVERIFY(! line->animating() && ! circle->animating());

        host->setParentItem(item);
        QVERIFY(line->animating() && circle->animating());
        outer->setHeight(300);
        viewport->setClip(true);
        QVERIFY(line->animating() && circle->animating());
        host->setX(400);
        QVERIFY(! line->animating() && ! circle->animating());
        host->setX(0);
        QVERIFY(line->animating() && circle->animating());
        host->setScale(0);
        QVERIFY(! line->animating() && ! circle->animating());
        host->setScale(1);
        QVERIFY(line->animating() && circle->animating());
        line->setWidth(0);
        QVERIFY(! line->animating() && circle->animating());
        line->setWidth(140);
        QVERIFY(line->animating() && circle->animating());
        host->setProperty("offset", 400);
        QTRY_VERIFY(! line->animating() && ! circle->animating());
        host->setProperty("offset", 0);
        QTRY_VERIFY(line->animating() && circle->animating());
        host->setParentItem(viewport->property("contentItem").value<QQuickItem*>());
        QTRY_VERIFY(! line->animating() && ! circle->animating());
        viewport->setProperty("contentY", 240);
        QTRY_VERIFY(line->animating() && circle->animating());
        QCOMPARE(line->value(), .5);
        QVERIFY(circle->running());
        QCOMPARE(circle->animationState(), ProgressIndicator::Running);
    }
    void sliderInput() {
        QTest::failOnWarning(QRegularExpression(".*"));
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                width: 320; height: 200
                MD.Slider { id: slider; objectName: "slider"; width: 300; from: 0; to: 1; value: .5; stepSize: .1 }
                MD.LinearIndicator { y: 60; width: 300; indeterminate: false; value: slider.value; animationsEnabled: false; completionBehavior: MD.LinearIndicator.Keep }
                MD.LinearIndicator { y: 80; width: 300; indeterminate: false; wavy: true; value: slider.value; animationsEnabled: false; completionBehavior: MD.LinearIndicator.Keep }
                MD.CircularIndicator { y: 100; indeterminate: false; value: slider.value; animationsEnabled: false; completionBehavior: MD.CircularIndicator.Keep }
                MD.CircularIndicator { x: 80; y: 100; indeterminate: false; wavy: true; value: slider.value; animationsEnabled: false; completionBehavior: MD.CircularIndicator.Keep }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QQuickWindow window;
        window.resize(320, 200);
        auto* item = qobject_cast<QQuickItem*>(root.get());
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* slider = root->findChild<QQuickItem*>("slider");
        QVERIFY(slider);
        auto verify = [&] {
            const auto value      = slider->property("value").toReal();
            const auto indicators = root->findChildren<ProgressIndicator*>();
            QCOMPARE(indicators.size(), 4);
            for (auto* indicator : indicators) QCOMPARE(indicator->displayedPosition(), value);
        };
        QTest::mousePress(
            &window, Qt::LeftButton, Qt::NoModifier, QPoint(150, slider->height() / 2));
        QTest::mouseMove(&window, QPoint(275, slider->height() / 2));
        QTest::mouseRelease(
            &window, Qt::LeftButton, Qt::NoModifier, QPoint(275, slider->height() / 2));
        QVERIFY(slider->property("value").toReal() > .5);
        verify();
        slider->forceActiveFocus();
        for (int i = 0; i < 11; ++i) QTest::keyClick(&window, Qt::Key_Left);
        QCOMPARE(slider->property("value").toReal(), 0.0);
        verify();
        for (int i = 0; i < 11; ++i) QTest::keyClick(&window, Qt::Key_Right);
        QCOMPARE(slider->property("value").toReal(), 1.0);
        verify();
        QTest::keyClick(&window, Qt::Key_Left);
        QVERIFY(slider->property("value").toReal() < 1);
        verify();
    }
};
int run_progress_indicator(int argc, char** argv) {
    ProgressIndicatorTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "progress_indicator.moc"
