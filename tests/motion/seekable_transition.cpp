#include "qml_material/anim/seekable_transition.hpp"
#include <QtTest>
using namespace qml_material;
class SeekableTransitionTest : public QObject {
    Q_OBJECT
private slots:
    void seekAndSettle() {
        SeekableTransition t;
        t.setManualClock(true);
        RealTrack x, alpha;
        x.setValues({ { "from", 0 }, { "to", 100 } });
        x.setSpec(Motion::tween(1000));
        alpha.setValues({ { "from", 0 }, { "to", 1 } });
        alpha.setSpec(Motion::tween(200));
        QVERIFY(t.addTrack(&x));
        QVERIFY(t.addTrack(&alpha));
        auto id = t.beginSeek("to");
        QVERIFY(! id.isEmpty());
        QVERIFY(t.seek(id, .7));
        QCOMPARE(x.value(), 70.0);
        QCOMPARE(alpha.value(), 1.0);
        QVERIFY(t.seek(id, .2));
        QCOMPARE(x.value(), 20.0);
        QCOMPARE(alpha.value(), 1.0);
        QVERIFY(t.seek(id, 1));
        QCOMPARE(t.currentState(), QString("from"));
        QVERIFY(t.active());
        QVERIFY(t.seek(id, .5));
        QSignalSpy ended(&t, &SeekableTransition::ended);
        t.settle(id, SeekableTransition::Origin, 0, Motion::tween(100));
        t.pause();
        t.advance(1000);
        QCOMPARE(t.fraction(), .5);
        QVERIFY(t.active());
        t.resume();
        t.advance(100);
        QTRY_VERIFY(! t.active());
        QCOMPARE(t.currentState(), QString("from"));
        QCOMPARE(x.value(), 0.0);
        const auto run = t.animateTo("to");
        t.advance(300);
        QCOMPARE(x.value(), 30.0);
        id = t.beginSeek("to");
        QCOMPARE(x.value(), 30.0);
        t.seek(id, .6);
        t.settle(id, SeekableTransition::Destination, 0, Motion::snap());
        QTRY_VERIFY(! t.active());
        QCOMPARE(t.currentState(), QString("to"));
        QCOMPARE(x.value(), 100.0);
        QVERIFY(! t.seek(run, .4));
    }
    void delayedEasingAndInvalidReplacement() {
        SeekableTransition transition;
        transition.setManualClock(true);
        RealTrack track;
        track.setValues({ { "from", 0 }, { "to", 100 } });
        track.setSpec(Motion::tween(200, QEasingCurve(QEasingCurve::InQuad), 100));
        transition.addTrack(&track);
        const auto id = transition.beginSeek("to");
        QVERIFY(transition.seek(id, 1.0 / 3));
        QCOMPARE(track.value(), 0.0);
        QVERIFY(transition.seek(id, 2.0 / 3));
        QCOMPARE(track.value(), 25.0);
        QVERIFY(transition.seek(id, .5));
        QCOMPARE(track.value(), 6.25);
        const auto value = track.value();
        QVERIFY(transition.animateTo("missing").isEmpty());
        QCOMPARE(track.value(), value);
        QVERIFY(transition.active());
        QVERIFY(
            transition.settle(id, SeekableTransition::Destination, 0, Motion::tween(-1)).isEmpty());
        QVERIFY(transition.seek(id, 1));
        QCOMPARE(track.value(), 100.0);
        transition.settle(id, SeekableTransition::Origin, 0, Motion::snap());
        QTRY_VERIFY(! transition.active());
        QCOMPARE(track.value(), 0.0);
    }
    void coherentSamplesAndDestruction() {
        SeekableTransition t;
        t.setManualClock(true);
        RealTrack x, y;
        for (auto* track : { &x, &y }) {
            track->setValues({ { "from", 0 }, { "to", 100 } });
            track->setSpec(Motion::tween(100));
            t.addTrack(track);
        }
        bool coherent = true;
        connect(&x, &RealTrack::valueChanged, &t, [&] {
            coherent &= x.value() == y.value();
        });
        t.animateTo("to");
        t.advance(50);
        QVERIFY(coherent);
        const auto old = x.value();
        x.setValues({ { "from", 0 }, { "to", 200 } });
        QCOMPARE(x.value(), old);
        t.advance(50);
        QTRY_VERIFY(! t.active());
        QCOMPARE(x.value(), 100.0);
        auto* extra = new RealTrack;
        extra->setValues({ { "from", 0 }, { "to", 1 } });
        t.addTrack(extra);
        t.animateTo("from");
        delete extra;
        QVERIFY(! t.active());
    }
};
int run_seekable_transition(int argc, char** argv) {
    SeekableTransitionTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "seekable_transition.moc"
