#include "qml_material/scrollable/flickable.hpp"
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QWheelEvent>
#include <QtTest>

using namespace qml_material;

class GeometryFlickable : public Flickable {
public:
    using Flickable::Flickable;
    using Flickable::mouseMoveEvent;
    using Flickable::mousePressEvent;
    using Flickable::touchEvent;
    using Flickable::touchUngrabEvent;
    using Flickable::updateContentGeometry;
};

class ScrollMotionTest : public QObject {
    Q_OBJECT
private slots:
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
            item.mousePressEvent(&press);
            QMouseEvent move(
                QEvent::MouseMove, moved, moved, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            item.mouseMoveEvent(&move);
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
        item.mouseMoveEvent(&stale);
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
        item.mousePressEvent(&press);
        QMouseEvent move(QEvent::MouseMove,
                         QPointF(50, 40),
                         QPointF(50, 40),
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        item.mouseMoveEvent(&move);
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
        item.mouseMoveEvent(&staleMove);
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
        QTest::newRow("ungrab") << 2;
    }
    void touchTermination() {
        QFETCH(int, termination);
        GeometryFlickable item;
        item.setSize({ 100, 100 });
        item.setContentHeight(1000);
        item.setFlickableDirection(Flickable::VerticalFlick);
        item.setSynchronousDrag(true);
        auto*      device = QTest::createTouchDevice();
        const auto touch  = [&](QEvent::Type type, QEventPoint::State state, int y, ulong time) {
            QEventPoint point(0, state, QPointF(50, y), QPointF(50, y));
            QTouchEvent event(type, device, Qt::NoModifier, { point });
            event.setTimestamp(time);
            item.touchEvent(&event);
        };
        touch(QEvent::TouchBegin, QEventPoint::State::Pressed, 90, 1000);
        touch(QEvent::TouchUpdate, QEventPoint::State::Updated, 40, 1050);
        QVERIFY(item.isDragging());
        QCOMPARE(item.contentY(), 50.0);
        if (termination == 0) {
            QTouchEvent event(QEvent::TouchCancel, device);
            item.touchEvent(&event);
            QVERIFY(event.isAccepted());
        } else if (termination == 2) {
            item.touchUngrabEvent();
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
            using Flickable::wheelEvent;
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
        inner.wheelEvent(&event);
        QVERIFY(event.isAccepted());
        QCOMPARE(inner.contentY(), 20.0);
        QCOMPARE(outer.contentY(), 30.0);
        QWheelEvent end(
            { 10, 10 }, { 10, 10 }, {}, {}, Qt::NoButton, Qt::NoModifier, Qt::ScrollEnd, false);
        inner.wheelEvent(&end);
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
        item.mousePressEvent(&press);
        QMouseEvent move(QEvent::MouseMove,
                         QPointF(50, 40),
                         QPointF(50, 40),
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        item.mouseMoveEvent(&move);
        QCOMPARE(item.contentY(), 50.0);
        item.updateContentGeometry({ 100, 1200 }, { 0, 200 });
        QMouseEvent next(QEvent::MouseMove,
                         QPointF(50, 30),
                         QPointF(50, 30),
                         Qt::NoButton,
                         Qt::LeftButton,
                         Qt::NoModifier);
        item.mouseMoveEvent(&next);
        QCOMPARE(item.contentY(), 260.0);
        QMouseEvent outside(QEvent::MouseMove,
                            QPointF(50, -10),
                            QPointF(50, -10),
                            Qt::NoButton,
                            Qt::LeftButton,
                            Qt::NoModifier);
        item.mouseMoveEvent(&outside);
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
