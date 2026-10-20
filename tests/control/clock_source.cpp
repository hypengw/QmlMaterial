#include <QtTest/QtTest>
#include <QQmlEngine>
#include <QTimer>
#include <QTimeZone>
#include "qml_material/clock/clock_source.hpp"

using namespace qml_material;
namespace
{
class SampledSource : public ClockSource {
public:
    QDateTime now = QDateTime::fromMSecsSinceEpoch(123456789, QTimeZone::UTC);
    using ClockSource::sample;
    QDateTime currentDateTime() const override { return now; }
};
} // namespace
class ClockSourceTest : public QObject {
    Q_OBJECT
private slots:
    void demandAndBoundaries() {
        SampledSource    source;
        QObject          minuteOwner, secondOwner;
        QList<QDateTime> minutes, seconds;
        auto             minute = source.subscribe(&minuteOwner, false, [&](auto time) {
            minutes.append(time);
        });
        auto             second = source.subscribe(&secondOwner, true, [&](auto time) {
            seconds.append(time);
        });
        auto*            timer  = source.findChild<QTimer*>();
        QVERIFY(timer);
        QVERIFY(! timer->isActive());
        minute->refresh();
        QCOMPARE(minutes.size(), 1);
        QVERIFY(! timer->isActive());
        minutes.clear();
        minute->setActive(true);
        QCOMPARE(minutes.size(), 1);
        QCOMPARE(minutes.last(), source.now);
        QCOMPARE(timer->interval(), 23211);
        second->setActive(true);
        QCOMPARE(timer->interval(), 211);
        source.now = source.now.addMSecs(211);
        source.sample();
        QCOMPARE(seconds.size(), 2);
        QCOMPARE(minutes.size(), 1);
        QCOMPARE(timer->interval(), 1000);
        source.now = QDateTime::fromMSecsSinceEpoch(123480000, QTimeZone::UTC);
        source.sample();
        QCOMPARE(minutes.size(), 2);
        QCOMPARE(timer->interval(), 1000);
        second.reset();
        QCOMPARE(timer->interval(), 60000);
        source.now = source.now.addSecs(9);
        minute->setSeconds(true);
        QCOMPARE(timer->interval(), 1000);
        minute->setSeconds(false);
        QCOMPARE(timer->interval(), 51000);
        minute.reset();
        QVERIFY(! timer->isActive());
    }
    void resamplingAndFixed() {
        SampledSource    source;
        QObject          context;
        QList<QDateTime> values;
        auto             subscription = source.subscribe(&context, false, [&](auto time) {
            values.append(time);
        });
        subscription->setActive(true);
        source.now = source.now.addDays(2).addSecs(17);
        source.sample();
        QCOMPARE(values.size(), 2);
        QCOMPARE(values.last(), source.now);
        source.now = source.now.addSecs(-3700);
        source.sample();
        QCOMPARE(values.size(), 3);
        QCOMPARE(values.last(), source.now);
        source.refresh();
        QCOMPARE(values.size(), 4);
        const auto fixed = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC);
        source.setFixedDateTime(fixed);
        QCOMPARE(values.size(), 4);
        source.setMode(ClockSource::Fixed);
        QCOMPARE(values.last(), fixed);
        QVERIFY(! source.findChild<QTimer*>()->isActive());
        source.setFixedDateTime(QDateTime());
        QCOMPARE(source.fixedDateTime(), fixed);
        source.refresh();
        QCOMPARE(source.dateTime(), fixed);
        subscription->setActive(false);
        const auto count = values.size();
        source.setFixedDateTime(fixed.addDays(1));
        QCOMPARE(values.size(), count);
        subscription->setActive(true);
        QCOMPARE(values.last(), fixed.addDays(1));
        source.setMode(ClockSource::System);
        QCOMPARE(values.last(), source.now);
        QVERIFY(source.findChild<QTimer*>()->isActive());
    }
    void negativeEpochAndUtc() {
        SampledSource source;
        QObject       context;
        auto          subscription = source.subscribe(&context, false, [](auto) {
        });
        source.now                 = QDateTime::fromMSecsSinceEpoch(-1, QTimeZone::UTC);
        subscription->setActive(true);
        QCOMPARE(source.findChild<QTimer*>()->interval(), 1);
        source.setFixedDateTime(
            QDateTime(QDate(2026, 10, 16), QTime(18, 9), QTimeZone::fromSecondsAheadOfUtc(28800)));
        source.setMode(ClockSource::Fixed);
        QCOMPARE(source.dateTime().time(), QTime(10, 9));
        QCOMPARE(source.dateTime().timeSpec(), Qt::UTC);
    }
    void ownershipAndReentry() {
        auto source       = std::make_unique<SampledSource>();
        auto context      = std::make_unique<QObject>();
        auto subscription = source->subscribe(context.get(), false, [](auto) {
        });
        subscription->setActive(true);
        context.reset();
        QVERIFY(! source->findChild<QTimer*>()->isActive());
        source.reset();
        subscription->setSeconds(true);
        subscription->setActive(false);
        subscription.reset();

        auto*                 reentrant = new SampledSource;
        QPointer<ClockSource> guard(reentrant);
        QObject               owner;
        auto                  s = reentrant->subscribe(&owner, false, [reentrant](auto) {
            delete reentrant;
        });
        s->setActive(true);
        QVERIFY(! guard);
        s.reset();

        SampledSource replacing;
        int           later  = 0;
        auto          first  = replacing.subscribe(&owner, true, [&](auto) {
            replacing.setMode(ClockSource::Fixed);
        });
        auto          second = replacing.subscribe(&owner, true, [&](auto) {
            ++later;
        });
        second->setActive(true);
        first->setActive(true);
        QCOMPARE(replacing.mode(), ClockSource::Fixed);
        QCOMPARE(later, 2);
    }
    void engineScope() {
        QQmlEngine first, second;
        auto*      a = ClockSource::shared(&first);
        QCOMPARE(a, ClockSource::shared(&first));
        QVERIFY(a != ClockSource::shared(&second));
        QVERIFY(! a->findChild<QTimer*>()->isActive());
        QPointer<ClockSource> guard;
        {
            QQmlEngine engine;
            guard = ClockSource::shared(&engine);
        }
        QVERIFY(! guard);
    }
};
int run_clock_source(int argc, char** argv) {
    ClockSourceTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "clock_source.moc"
