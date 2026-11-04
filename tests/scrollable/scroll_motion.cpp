#include "qml_material/scrollable/flickable.hpp"
#include "qml_material/input/nested_scroll.hpp"
#include "../../src/input/scroll_wheel_p.hpp"
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QStyleHints>
#include <QWheelEvent>
#include <QtTest>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>

using namespace qml_material;

class GeometryFlickable : public Flickable {
public:
    using Flickable::Flickable;
    using Flickable::updateContentGeometry;
};

class ScrollMotionTest : public QObject {
    Q_OBJECT
    QQuickWindow inputWindow;
    void         deliver(Flickable& item, QEvent* event) {
        if (! item.window()) item.setParentItem(inputWindow.contentItem());
        QCoreApplication::sendEvent(&inputWindow, event);
    }
private slots:
    void init() {
        inputWindow.resize(100, 100);
        inputWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&inputWindow));
    }
    void cleanup() { inputWindow.hide(); }

    void actualConsumption() {
        Flickable item;
        item.setSize({ 100, 100 });
        item.setContentWidth(150);
        item.setContentHeight(180);
        item.setFlickableDirection(Flickable::HorizontalAndVerticalFlick);
        QSignalSpy released(&item, &Flickable::dragReleased);
        item.beginScroll(Flickable::ScrollSource::Drag);
        const auto first =
            item.applyScrollDelta({ 30, 70 }, Flickable::ScrollSource::Drag, { 300, 700 });
        QCOMPARE(first.consumed, QPointF(30, 70));
        QCOMPARE(first.remaining, QPointF());
        const auto second = item.applyScrollDelta({ 30, 30 }, Flickable::ScrollSource::Drag);
        QCOMPARE(second.consumed, QPointF(20, 10));
        QCOMPARE(second.remaining, QPointF(10, 20));
        QCOMPARE(QPointF(item.contentX(), item.contentY()), first.consumed + second.consumed);
        item.releaseScroll({ 300, 0 });
        QCOMPARE(released.size(), 1);
        QVERIFY(! item.isDragging());
        QVERIFY(item.isMoving());
        item.endScroll(Flickable::ScrollEnd::Finished);
        QVERIFY(! item.isMoving());
        item.beginScroll(Flickable::ScrollSource::Drag);
        item.applyScrollDelta({ -10, 0 }, Flickable::ScrollSource::Drag);
        item.endScroll(Flickable::ScrollEnd::Cancelled);
        QCOMPARE(released.size(), 1);
        QVERIFY(! item.isDragging());
    }
    void consumptionInterruptedByCallback() {
        Flickable item;
        item.setSize({ 100, 100 });
        item.setContentHeight(1000);
        item.beginScroll(Flickable::ScrollSource::Drag);
        bool interrupted = false;
        connect(&item, &Flickable::contentYChanged, &item, [&] {
            if (! interrupted) {
                interrupted = true;
                item.setContentY(200);
            }
        });
        item.applyScrollDelta({ 0, 50 }, Flickable::ScrollSource::Drag, { 0, 1000 });
        QCOMPARE(item.contentY(), 200.0);
        QVERIFY(! item.isDragging());
        QVERIFY(! item.isMoving());
    }
    void directPositionReplacesInput_data() {
        QTest::addColumn<bool>("horizontal");
        QTest::addColumn<bool>("drag");
        for (bool horizontal : { false, true }) {
            QTest::newRow(horizontal ? "horizontal-drag" : "vertical-drag") << horizontal << true;
            QTest::newRow(horizontal ? "horizontal-platform" : "vertical-platform")
                << horizontal << false;
        }
    }
    void directPositionReplacesInput() {
        QFETCH(bool, horizontal);
        QFETCH(bool, drag);
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.setContentWidth(2000);
        item.setContentHeight(2000);
        item.setFlickableDirection(Flickable::HorizontalAndVerticalFlick);
        item.setSynchronousDrag(true);
        const QPointF start(90, 90);
        const QPointF moved = horizontal ? QPointF(40, 90) : QPointF(90, 40);
        if (drag) {
            QMouseEvent press(QEvent::MouseButtonPress,
                              start,
                              start,
                              Qt::LeftButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
            deliver(item, &press);
            QMouseEvent move(
                QEvent::MouseMove, moved, moved, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            deliver(item, &move);
            QVERIFY(item.isDragging());
        } else {
            item.consumeScroll(horizontal ? QPointF(50, 0) : QPointF(0, 50),
                               Flickable::ScrollInput::Direct,
                               Qt::ScrollMomentum);
        }
        QVERIFY(item.isMoving());
        if (horizontal)
            item.setContentX(300);
        else
            item.setContentY(300);
        QVERIFY(! item.isMoving());
        QVERIFY(! item.isDragging());
        QVERIFY(! item.keepMouseGrab());
        QVERIFY(! item.keepTouchGrab());
        QMouseEvent stale(
            QEvent::MouseMove, moved, moved, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        deliver(item, &stale);
        QCOMPARE(horizontal ? item.contentX() : item.contentY(), 300.0);
        QCOMPARE(horizontal ? item.horizontalVelocity() : item.verticalVelocity(), 0.0);
    }
    void scrollReplacesDrag() {
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.setContentHeight(2000);
        item.setFlickableDirection(Flickable::VerticalFlick);
        item.setSynchronousDrag(true);
        QMouseEvent press(QEvent::MouseButtonPress,
                          QPointF(50, 90),
                          QPointF(50, 90),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        deliver(item, &press);
        QMouseEvent move(QEvent::MouseMove,
                         QPointF(50, 40),
                         QPointF(50, 40),
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        deliver(item, &move);
        QVERIFY(item.isDragging());
        const auto consumed =
            item.consumeScroll({ 0, 20 }, Flickable::ScrollInput::Direct, Qt::ScrollBegin);
        QCOMPARE(consumed.consumed.y(), 20.0);
        QCOMPARE(item.contentY(), 70.0);
        QVERIFY(! item.isDragging());
        QVERIFY(! item.keepMouseGrab());
        item.consumeScroll({}, Flickable::ScrollInput::Direct, Qt::ScrollEnd);
        QVERIFY(! item.isMoving());
        QMouseEvent staleMove(QEvent::MouseMove,
                              QPointF(50, 10),
                              QPointF(50, 10),
                              Qt::NoButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
        deliver(item, &staleMove);
        QCOMPARE(item.contentY(), 70.0);
    }
    void samplingCadence_data() {
        QTest::addColumn<int>("hz");
        for (int hz : { 60, 90, 120, 144 }) QTest::newRow(qPrintable(QString::number(hz))) << hz;
    }
    void samplingCadence() {
        QFETCH(int, hz);
        ScrollMotion motion;
        motion.fling(10, 1000, 1000, 0);
        for (int frame = 0; frame < hz * 2; ++frame) {
            const double now = double(frame) / hz;
            if (now > 0.2 && now < 0.6) continue;
            const auto   sample  = motion.sample(now);
            const double elapsed = std::min(1.0, now);
            QCOMPARE(sample.position, 10 + 1000 * elapsed - 500 * elapsed * elapsed);
            QCOMPARE(sample.velocity, 1000 - 1000 * elapsed);
            QCOMPARE(sample.finished, now >= 1);
        }
        motion.smooth(0, 0, 500, 0);
        for (int frame = 0; frame < hz * 2; ++frame) {
            const double now = double(frame) / hz;
            if (now > 0.1 && now < 0.5) continue;
            ScrollMotion reference;
            reference.smooth(0, 0, 500, 0);
            QCOMPARE(motion.sample(now).position, reference.sample(now).position);
            QCOMPARE(motion.sample(now).velocity, reference.sample(now).velocity);
        }
        QCOMPARE(motion.sample(2).position, 500.0);
        QVERIFY(motion.sample(2).finished);
    }
    void windowFrames() {
        QQuickWindow window;
        window.setGeometry(0, 0, 200, 200);
        Flickable item(window.contentItem());
        item.setSize({ 200, 200 });
        item.setContentHeight(2000);
        item.setFlickableDirection(Flickable::VerticalFlick);
        item.setFlickDeceleration(5000);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy frames(&window, &QQuickWindow::afterAnimating);
        QSignalSpy ended(&item, &Flickable::movementEnded);
        item.flick(0, -1000);
        QTRY_VERIFY_WITH_TIMEOUT(! item.isMoving(), 2000);
        QCOMPARE(item.contentY(), 100.0);
        QCOMPARE(ended.size(), 1);
        QVERIFY(frames.size() > 1);
    }
    void touchTermination_data() {
        QTest::addColumn<int>("termination");
        QTest::newRow("empty-cancel") << 0;
        QTest::newRow("release-outside") << 1;
        QTest::newRow("window-deactivate") << 2;
    }
    void touchTermination() {
        QFETCH(int, termination);
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.setContentHeight(1000);
        item.setFlickableDirection(Flickable::VerticalFlick);
        item.setSynchronousDrag(true);
        item.setParentItem(inputWindow.contentItem());
        item.setMaximumFlickVelocity(0);
        static auto* device = QTest::createTouchDevice();
        const auto   touch  = [&](QEvent::Type, QEventPoint::State state, int y, ulong) {
            auto sequence = QTest::touchEvent(&inputWindow, device);
            if (state == QEventPoint::Pressed)
                sequence.press(0, { 50, y });
            else if (state == QEventPoint::Released)
                sequence.release(0, { 50, y });
            else
                sequence.move(0, { 50, y });
            sequence.commit();
            QQuickWindowPrivate::get(&inputWindow)
                ->deliveryAgentPrivate()
                ->flushFrameSynchronousEvents(&inputWindow);
        };
        touch(QEvent::TouchBegin, QEventPoint::State::Pressed, 90, 1000);
        touch(QEvent::TouchUpdate, QEventPoint::State::Updated, 40, 1050);
        QVERIFY(item.isDragging());
        QCOMPARE(item.contentY(), 50.0);
        if (termination == 0) {
            QTouchEvent event(QEvent::TouchCancel, device);
            deliver(item, &event);
        } else if (termination == 2) {
            QEvent event(QEvent::WindowDeactivate);
            deliver(item, &event);
        } else {
            touch(QEvent::TouchUpdate, QEventPoint::State::Updated, -20, 1100);
            QCOMPARE(item.contentY(), 110.0);
            touch(QEvent::TouchEnd, QEventPoint::State::Released, -20, 1500);
        }
        QVERIFY(! item.isDragging());
        QVERIFY(! item.isMoving());
        QVERIFY(! item.keepMouseGrab());
        const qreal offset = item.contentY();
        touch(QEvent::TouchUpdate, QEventPoint::State::Updated, 20, 1600);
        QCOMPARE(item.contentY(), offset);
        QTest::touchEvent(&inputWindow, device).release(0, { 50, 20 }).commit();
    }
    void analyticFling() {
        ScrollMotion motion;
        motion.fling(10, 1000, 1000, 5);
        QCOMPARE(motion.sample(5.5).position, 385.0);
        QCOMPARE(motion.sample(5.5).velocity, 500.0);
        for (int i = 0; i < 144; ++i) motion.sample(5 + i / 144.0);
        QCOMPARE(motion.sample(5.5).position, 385.0);
        QCOMPARE(motion.sample(20).position, 510.0);
        QVERIFY(motion.sample(20).finished);
        motion.fling(10, -1000, 1000, 5);
        QCOMPARE(motion.sample(6).position, -490.0);
    }
    void wheelDistanceParsing_data() {
        QTest::addColumn<QByteArray>("value");
        QTest::addColumn<int>("expected");
        QTest::newRow("default") << QByteArray() << int(WheelDistanceMode::Steps);
        QTest::newRow("platform") << QByteArray("platform") << int(WheelDistanceMode::Platform);
        QTest::newRow("steps") << QByteArray("steps") << int(WheelDistanceMode::Steps);
        QTest::newRow("case-and-space")
            << QByteArray(" PLATFORM ") << int(WheelDistanceMode::Platform);
        QTest::newRow("empty-space") << QByteArray("  ") << int(WheelDistanceMode::Steps);
        QTest::newRow("invalid") << QByteArray("invalid") << -1;
    }
    void wheelDistanceParsing() {
        QFETCH(QByteArray, value);
        QFETCH(int, expected);
        const auto mode = parseWheelDistanceMode(value);
        QCOMPARE(bool(mode), expected >= 0);
        if (mode) QCOMPARE(int(*mode), expected);
    }
    void wheelInputDistance_data() {
        QTest::addColumn<bool>("steps");
        QTest::addColumn<int>("phase");
        QTest::addColumn<QPoint>("pixels");
        QTest::addColumn<QPoint>("angles");
        QTest::addColumn<QPointF>("expected");
        QTest::addColumn<bool>("smooth");
        QTest::newRow("platform-mouse") << false << int(Qt::NoScrollPhase) << QPoint(0, -15)
                                        << QPoint(0, -120) << QPointF(0, 15) << true;
        QTest::newRow("platform-speed") << false << int(Qt::NoScrollPhase) << QPoint(0, -30)
                                        << QPoint(0, -120) << QPointF(0, 30) << true;
        QTest::newRow("steps-mouse") << true << int(Qt::NoScrollPhase) << QPoint(0, -30)
                                     << QPoint(0, -120) << QPointF(0, 72) << true;
        QTest::newRow("platform-fallback") << false << int(Qt::NoScrollPhase) << QPoint()
                                           << QPoint(0, -120) << QPointF(0, 72) << true;
        QTest::newRow("steps-fallback") << true << int(Qt::NoScrollPhase) << QPoint()
                                        << QPoint(0, -120) << QPointF(0, 72) << true;
        QTest::newRow("precision-platform") << false << int(Qt::ScrollUpdate) << QPoint(0, -15)
                                            << QPoint(0, -120) << QPointF(0, 15) << false;
        QTest::newRow("precision-steps") << true << int(Qt::ScrollUpdate) << QPoint(0, -15)
                                         << QPoint(0, -120) << QPointF(0, 15) << false;
        QTest::newRow("precision-fallback") << true << int(Qt::ScrollUpdate) << QPoint()
                                            << QPoint(0, -120) << QPointF(0, 72) << false;
        QTest::newRow("momentum") << true << int(Qt::ScrollMomentum) << QPoint(0, -8)
                                  << QPoint(0, -64) << QPointF(0, 8) << false;
        QTest::newRow("zero-mouse")
            << false << int(Qt::NoScrollPhase) << QPoint() << QPoint() << QPointF() << true;
        QTest::newRow("zero-end") << false << int(Qt::ScrollEnd) << QPoint() << QPoint()
                                  << QPointF() << false;
        QTest::newRow("horizontal") << false << int(Qt::NoScrollPhase) << QPoint(-15, 0)
                                    << QPoint(-120, 0) << QPointF(15, 0) << true;
        QTest::newRow("pixel-vector-not-axis-fallback")
            << false << int(Qt::NoScrollPhase) << QPoint(0, -15) << QPoint(-120, -120)
            << QPointF(0, 15) << true;
        QTest::newRow("reverse") << false << int(Qt::NoScrollPhase) << QPoint(0, 15)
                                 << QPoint(0, 120) << QPointF(0, -15) << true;
        QTest::newRow("coalesced") << false << int(Qt::NoScrollPhase) << QPoint(0, -30)
                                   << QPoint(0, -240) << QPointF(0, 30) << true;
    }
    void wheelInputDistance() {
        QFETCH(bool, steps);
        QFETCH(int, phase);
        QFETCH(QPoint, pixels);
        QFETCH(QPoint, angles);
        QFETCH(QPointF, expected);
        QFETCH(bool, smooth);
        const QWheelEvent event(
            {}, {}, pixels, angles, Qt::NoButton, Qt::NoModifier, Qt::ScrollPhase(phase), false);
        const auto input =
            wheelInput(event, steps ? WheelDistanceMode::Steps : WheelDistanceMode::Platform, 3);
        QCOMPARE(input.delta, expected);
        QCOMPARE(input.smooth, smooth);
    }
    void mouseWheelPixels_data() {
        QTest::addColumn<QPoint>("pixels");
        QTest::addColumn<QPoint>("angles");
        QTest::newRow("angle-only") << QPoint() << QPoint(0, -120);
        QTest::newRow("mouse-pixels") << QPoint(0, -15) << QPoint(0, -120);
        QTest::newRow("horizontal-mouse-pixels") << QPoint(-15, 0) << QPoint(-120, 0);
        QTest::newRow("coalesced-mouse-pixels") << QPoint(0, -30) << QPoint(0, -240);
        QTest::newRow("high-resolution-mouse-pixels") << QPoint(0, -1) << QPoint(0, -8);
        QTest::newRow("reverse-mouse-pixels") << QPoint(0, 15) << QPoint(0, 120);
    }
    void mouseWheelPixels() {
        QFETCH(QPoint, pixels);
        QFETCH(QPoint, angles);
        Flickable item(inputWindow.contentItem());
        item.setSize({ 100, 100 });
        item.setContentWidth(2000);
        item.setContentHeight(2000);
        item.setFlickableDirection(Flickable::HorizontalAndVerticalFlick);
        item.setContentX(500);
        item.setContentY(500);
        auto* config =
            qobject_cast<NestedScroll*>(qmlAttachedPropertiesObject<NestedScroll>(&item));
        QSignalSpy started(config, &NestedScroll::scrollStarted);
        QSignalSpy finished(config, &NestedScroll::scrollFinished);
        for (int i = 0; i < 4; ++i) {
            QWheelEvent event({ 50, 50 },
                              inputWindow.mapToGlobal(QPoint(50, 50)),
                              pixels,
                              angles,
                              Qt::NoButton,
                              Qt::NoModifier,
                              Qt::NoScrollPhase,
                              false);
            deliver(item, &event);
            QVERIFY(event.isAccepted());
            QCOMPARE(started.size(), 1);
            QVERIFY(finished.isEmpty());
        }
        const auto mode = parseWheelDistanceMode(qgetenv("QML_MATERIAL_WHEEL_DISTANCE"))
                              .value_or(WheelDistanceMode::Steps);
        const auto distance =
            mode == WheelDistanceMode::Platform && ! pixels.isNull()
                ? -QPointF(pixels)
                : -QPointF(angles) / 120 * (24 * qGuiApp->styleHints()->wheelScrollLines());
        const auto expected = QPointF(500, 500) + distance * 4;
        QTRY_VERIFY(std::abs(item.contentX() - expected.x()) < 0.001 &&
                    std::abs(item.contentY() - expected.y()) < 0.001);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(! item.isMoving());
        QCOMPARE(finished.first().first().toBool(), false);
    }
    void wheelDuration_data() {
        QTest::addColumn<double>("distance");
        QTest::addColumn<double>("duration");
        QTest::newRow("notch") << 72.0 << 0.2;
        QTest::newRow("medium") << 300.0 << 0.15;
        QTest::newRow("fast") << 480.0 << 0.1;
        QTest::newRow("large-reverse") << -1000.0 << 0.1;
    }
    void wheelDuration() {
        QFETCH(double, distance);
        QFETCH(double, duration);
        ScrollMotion motion;
        motion.wheel(0, distance, 0);
        QCOMPARE(motion.mode(), ScrollMotion::Mode::Wheel);
        QVERIFY(std::abs(motion.sample(duration / 2).position - distance / 2) < 1e-8);
        QVERIFY(! motion.sample(duration - 0.00001).finished);
        QCOMPARE(motion.sample(duration).position, distance);
        QCOMPARE(motion.sample(duration).velocity, 0.0);
        QVERIFY(motion.sample(duration).finished);
    }
    void wheelRetargetBetweenFrames() {
        ScrollMotion motion;
        motion.wheel(0, 72, 0);
        const auto shown  = motion.sample(0.016);
        const auto before = motion.sample(0.025);
        QVERIFY(before.position > shown.position);
        motion.wheel(shown.position, 144, 0.025);
        QCOMPARE(motion.sample(0.025).position, before.position);
        QCOMPARE(motion.sample(0.025).velocity, before.velocity);
        const auto sameTarget = motion.sample(0.03);
        motion.wheel(shown.position, 144, 0.03);
        QCOMPARE(motion.sample(0.03).position, sameTarget.position);
        QCOMPARE(motion.sample(0.03).velocity, sameTarget.velocity);
        motion.translate(200);
        QCOMPARE(motion.sample(0.03).position, sameTarget.position + 200);
        QCOMPARE(motion.sample(1).position, 344.0);
    }
    void freeSpinningWheel_data() {
        QTest::addColumn<int>("hz");
        for (int hz : { 60, 90, 120, 144 }) QTest::newRow(qPrintable(QString::number(hz))) << hz;
    }
    void freeSpinningWheel() {
        QFETCH(int, hz);
        ScrollMotion motion;
        double       shown = 0, target = 0, previous = 0;
        int          nextFrame = 0;
        for (int event = 0; event < 1000; ++event) {
            const double time = event / 1000.0;
            if (time >= double(nextFrame) / hz) {
                shown = motion.active() ? motion.sample(time).position : 0;
                ++nextFrame;
            }
            const auto before =
                motion.active() ? motion.sample(time) : ScrollMotion::Sample { 0, 0, true };
            // High-resolution wheel events accelerate and then decay during free spin.
            target += event < 100 ? 0.1 + event * 0.1 : 10 * std::exp(-(event - 100) / 180.0);
            motion.wheel(shown, target, time);
            const auto after = motion.sample(time);
            QCOMPARE(after.position, before.position);
            QVERIFY(std::abs(after.velocity - before.velocity) < 1e-8);
            QVERIFY(after.position >= previous);
            QVERIFY(after.position <= target);
            previous = after.position;
        }
        QCOMPARE(motion.sample(1.2).position, target);
        QVERIFY(motion.sample(1.2).finished);
    }
    void wheelVelocityBoundAndReverse() {
        ScrollMotion motion;
        motion.wheel(0, 1000, 0);
        const auto fast = motion.sample(0.05);
        QVERIFY(fast.velocity > 1000);
        motion.wheel(0, fast.position + 1, 0.05);
        QCOMPARE(motion.sample(0.05).velocity, fast.velocity);
        for (int i = 0; i <= 100; ++i) {
            const auto sample = motion.sample(0.05 + i * 0.0001);
            QVERIFY(sample.position >= fast.position);
            QVERIFY(sample.position <= fast.position + 1.000001);
        }
        QVERIFY(motion.sample(0.06).finished);
        motion.wheel(fast.position + 1, 0, 0.06);
        QCOMPARE(motion.sample(0.06).position, fast.position + 1);
        QCOMPARE(motion.sample(0.3).position, 0.0);
        QVERIFY(motion.sample(0.3).finished);
        motion.wheel(0, 100, 0.4);
        const auto forward = motion.sample(0.45);
        motion.wheel(0, -100, 0.45);
        QCOMPARE(motion.sample(0.45).position, forward.position);
        QCOMPARE(motion.sample(0.45).velocity, forward.velocity);
        QCOMPARE(motion.sample(0.7).position, -100.0);
        motion.stop();
        motion.wheel(100, 100, 1);
        QCOMPARE(motion.sample(1).position, 100.0);
        QVERIFY(motion.sample(1).finished);
    }
    void wheelBurstAndDirectTakeover() {
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        Flickable           item(window.contentItem());
        item.setSize({ 100, 100 });
        item.setContentWidth(2000);
        item.setContentHeight(2000);
        item.setFlickableDirection(Flickable::HorizontalAndVerticalFlick);
        for (int i = 0; i < 100; ++i) {
            const auto result = item.consumeScroll({ 0.75, 0.75 }, Flickable::ScrollInput::Smooth);
            QCOMPARE(result.consumed, QPointF(0.75, 0.75));
        }
        QVERIFY(item.isMoving());
        // Precision input replaces queued wheel motion, even without ScrollBegin.
        const qreal before  = item.contentY();
        const qreal beforeX = item.contentX();
        item.consumeScroll({ 0, 10 }, Flickable::ScrollInput::Direct, Qt::ScrollMomentum);
        QCOMPARE(item.contentY(), before + 10);
        item.consumeScroll({}, Flickable::ScrollInput::Direct, Qt::ScrollEnd);
        QVERIFY(! item.isMoving());
        render.polishItems();
        QCOMPARE(item.contentY(), before + 10);
        QCOMPARE(item.contentX(), beforeX);
    }
    void translationAndRetarget() {
        ScrollMotion motion;
        motion.fling(0, 1000, 1000, 0);
        const auto before = motion.sample(0.2);
        motion.translate(300);
        QCOMPARE(motion.sample(0.2).position, before.position + 300);
        QCOMPARE(motion.sample(0.2).velocity, before.velocity);
        motion.smooth(before.position, before.velocity, 700, 0.2);
        const auto moving = motion.sample(0.3);
        motion.smooth(moving.position, moving.velocity, 900, 0.3);
        QCOMPARE(motion.sample(0.3).position, moving.position);
        QVERIFY(qAbs(motion.sample(0.3).velocity - moving.velocity) < 1e-9);
        QCOMPARE(motion.sample(3).position, 900.0);
        QVERIFY(motion.sample(3).finished);
    }
    void unavailableCancels() {
        QQuickRenderControl render;
        QQuickWindow        window(&render);
        Flickable           item(window.contentItem());
        item.setSize({ 100, 100 });
        item.setContentHeight(2000);
        item.setFlickableDirection(Flickable::VerticalFlick);
        item.flick(0, -1000);
        QVERIFY(item.isMoving());
        item.setVisible(false);
        QVERIFY(! item.isMoving());
        item.setVisible(true);
        render.polishItems();
        QCOMPARE(item.contentY(), 0.0);
        item.flick(0, -1000);
        item.setInteractive(false);
        QVERIFY(! item.isMoving());
        item.setInteractive(true);
        item.flick(0, -1000);
        item.setParentItem(nullptr);
        QVERIFY(! item.isMoving());
        item.flick(0, -1000);
        QVERIFY(! item.isMoving());
    }
    void deltaConsumption() {
        Flickable item;
        item.setSize({ 100, 100 });
        item.setContentHeight(150);
        item.setFlickableDirection(Flickable::VerticalFlick);
        const auto result = item.consumeScroll({ 15, 80 }, Flickable::ScrollInput::Direct);
        QCOMPARE(result.consumed, QPointF(0, 50));
        QCOMPARE(result.remaining, QPointF(15, 30));
        QCOMPARE(item.contentY(), 50.0);
        QVERIFY(! item.isMoving());
        item.consumeScroll({ 0, -10 }, Flickable::ScrollInput::Direct, Qt::ScrollBegin);
        QVERIFY(item.isMoving());
        item.consumeScroll({ 0, -10 }, Flickable::ScrollInput::Direct, Qt::ScrollMomentum);
        QVERIFY(item.isMoving());
        QVERIFY(! item.isFlicking());
        item.consumeScroll({}, Flickable::ScrollInput::Direct, Qt::ScrollEnd);
        QVERIFY(! item.isMoving());
        QCOMPARE(item.contentY(), 30.0);
    }
    void nestedWheel() {
        class WheelItem : public Flickable {
        public:
            using Flickable::Flickable;
        };
        WheelItem outer;
        outer.setSize({ 100, 100 });
        outer.setContentHeight(1000);
        outer.setFlickableDirection(Flickable::VerticalFlick);
        WheelItem inner(outer.contentItem());
        inner.setSize({ 100, 100 });
        inner.setContentHeight(120);
        inner.setFlickableDirection(Flickable::VerticalFlick);
        QWheelEvent event({ 10, 10 },
                          { 10, 10 },
                          { 0, -50 },
                          {},
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::ScrollUpdate,
                          true);
        deliver(outer, &event);
        QVERIFY(event.isAccepted());
        QCOMPARE(inner.contentY(), 20.0);
        QCOMPARE(outer.contentY(), 30.0);
        QWheelEvent end(
            { 10, 10 }, { 10, 10 }, {}, {}, Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false);
        deliver(outer, &end);
        QVERIFY(! inner.isMoving());
        QVERIFY(! outer.isMoving());
    }
    void atomicGeometry() {
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.updateContentGeometry({ 500, 1000 }, { 100, 700 });
        bool notified = false;
        connect(&item, &Flickable::contentHeightChanged, &item, [&] {
            notified = true;
            QCOMPARE(item.contentWidth(), 300.0);
            QCOMPARE(item.contentHeight(), 600.0);
            QCOMPARE(item.contentX(), 120.0);
            QCOMPARE(item.contentY(), 450.0);
        });
        item.updateContentGeometry({ 300, 600 }, { 20, -250 });
        QVERIFY(notified);
        QCOMPARE(item.contentItem()->position(), QPointF(-120, -450));
    }
    void geometryReentry() {
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.updateContentGeometry({ 100, 1000 });
        bool       entered = false;
        QSignalSpy heights(&item, &Flickable::contentHeightChanged);
        connect(&item, &Flickable::contentWidthChanged, &item, [&] {
            if (entered) return;
            entered = true;
            item.updateContentGeometry({ 400, 900 }, { 0, 10 });
        });
        item.updateContentGeometry({ 300, 800 }, { 0, 100 });
        QCOMPARE(item.contentY(), 110.0);
        QCOMPARE(item.contentItem()->size(), QSizeF(400, 900));
        QCOMPARE(item.contentItem()->y(), -110.0);
        QCOMPARE(heights.size(), 1);
    }
    void geometryPreservesDrag() {
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.setContentHeight(1000);
        item.setFlickableDirection(Flickable::VerticalFlick);
        item.setSynchronousDrag(true);
        QMouseEvent press(QEvent::MouseButtonPress,
                          QPointF(50, 90),
                          QPointF(50, 90),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
        deliver(item, &press);
        QMouseEvent move(QEvent::MouseMove,
                         QPointF(50, 40),
                         QPointF(50, 40),
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        deliver(item, &move);
        QCOMPARE(item.contentY(), 50.0);
        item.updateContentGeometry({ 100, 1200 }, { 0, 200 });
        QMouseEvent next(QEvent::MouseMove,
                         QPointF(50, 30),
                         QPointF(50, 30),
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        deliver(item, &next);
        QCOMPARE(item.contentY(), 260.0);
        QMouseEvent outside(QEvent::MouseMove,
                            QPointF(50, -10),
                            QPointF(50, -10),
                            Qt::NoButton,
                            Qt::LeftButton,
                            Qt::NoModifier);
        deliver(item, &outside);
        QCOMPARE(item.contentY(), 300.0);
    }
    void geometryDeletion() {
        auto* item = new GeometryFlickable;
        item->setSize({ 100, 100 });
        QPointer<GeometryFlickable> guard(item);
        connect(item, &Flickable::contentHeightChanged, item, [item] {
            delete item;
        });
        item->updateContentGeometry({ 100, 1000 }, { 0, 200 });
        QVERIFY(! guard);
        item = new GeometryFlickable;
        item->setSize({ 100, 100 });
        guard = item;
        connect(item, &Flickable::atYEndChanged, item, [item] {
            delete item;
        });
        item->updateContentGeometry({ 100, 1000 });
        QVERIFY(! guard);
    }
};

int run_scroll_motion(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi, true);
    ScrollMotionTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "scroll_motion.moc"
