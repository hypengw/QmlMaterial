#include <QtTest/QtTest>
#include <array>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlListReference>
#include <QQuickWindow>
#include <QTimeZone>
#include <QTimer>
#include <private/qqmldata_p.h>
#include "qml_material/control/clock.hpp"
#include "qml_material/control/segmented_button_group.hpp"
#include "qml_material/core/enum.hpp"
#include "qml_material/shape/material_shapes.hpp"
#include "qml_material/token/color.hpp"

using namespace qml_material;
namespace
{
class TestClock : public Clock {
public:
    using Clock::Clock;
    using Clock::componentComplete;
};
class SampledSource : public ClockSource {
public:
    QDateTime now = QDateTime::fromMSecsSinceEpoch(60000, QTimeZone::UTC);
    using ClockSource::sample;
    QDateTime currentDateTime() const override { return now; }
};
QDateTime utc(const char* text) {
    return QDateTime::fromString(QString::fromLatin1(text), Qt::ISODate).toUTC();
}

void polish(QQuickItem* item) {
    item->ensurePolished();
    for (auto* child : item->childItems()) polish(child);
}
} // namespace

class ClockPresetsTest : public QObject {
    Q_OBJECT

private slots:
    void init() {
        QTest::failOnWarning(QRegularExpression(
            ".*(Binding loop|Unable to assign|Cannot assign|TypeError|ReferenceError).*"));
    }

    void fieldsAndLocale_data() {
        QTest::addColumn<QString>("localeName");
        QTest::addColumn<int>("hour");
        QTest::addColumn<TimeState::HourFormat>("format");
        QTest::newRow("midnight") << QStringLiteral("en_US") << 0 << TimeState::Hour12;
        QTest::newRow("noon") << QStringLiteral("en_US") << 12 << TimeState::Hour12;
        QTest::newRow("24hour") << QStringLiteral("en_US") << 23 << TimeState::Hour24;
        QTest::newRow("chinese") << QStringLiteral("zh_CN") << 18 << TimeState::LocaleFormat;
        QTest::newRow("arabic") << QStringLiteral("ar_EG") << 18 << TimeState::Hour12;
    }

    void fieldsAndLocale() {
        QFETCH(QString, localeName);
        QFETCH(int, hour);
        QFETCH(TimeState::HourFormat, format);
        ClockSource source;
        source.setMode(ClockSource::Fixed);
        const auto instant = QDateTime(QDate(2026, 12, 31), QTime(hour, 9, 59), QTimeZone::UTC);
        source.setFixedDateTime(instant);
        TestClock clock;
        clock.setSource(&source);
        clock.setLocale(QLocale(localeName));
        clock.setTimeZoneId("UTC");
        clock.setHourFormat(format);
        clock.setShowSeconds(true);
        clock.setShowDate(true);
        clock.componentComplete();
        QCOMPARE(clock.hour(), hour);
        QCOMPARE(clock.minute(), 9);
        QCOMPARE(clock.second(), 59);
        const auto expectedHour = clock.is24Hour() ? hour : (hour % 12 ? hour % 12 : 12);
        auto       hourString   = clock.locale().toString(expectedHour);
        if (expectedHour < 10) hourString.prepend(clock.locale().zeroDigit());
        QCOMPARE(clock.hourText(), hourString);
        QCOMPARE(clock.minuteText(), clock.locale().zeroDigit() + clock.locale().toString(9));
        QCOMPARE(clock.secondText(), clock.locale().toString(59));
        QCOMPARE(
            clock.displayText(),
            clock.locale().toString(instant.time(), clock.is24Hour() ? "HH:mm:ss" : "hh:mm:ss AP"));
        QCOMPARE(clock.dateText(), clock.locale().toString(instant.date(), QLocale::LongFormat));
        QCOMPARE(clock.dayText(), clock.locale().toString(instant.date(), "dd"));
        QCOMPARE(clock.monthText(), clock.locale().toString(instant.date(), "MM"));
        QCOMPARE(
            clock.weekdayText(),
            clock.locale().standaloneDayName(instant.date().dayOfWeek(), QLocale::ShortFormat));
        QCOMPARE(clock.compactDateText(), clock.locale().toString(instant.date(), "ddd, MMM d"));
        QCOMPARE(clock.periodText(),
                 clock.is24Hour() ? QString()
                 : hour < 12      ? clock.locale().amText()
                                  : clock.locale().pmText());
        QCOMPARE(clock.hourAngle(), (hour % 12 + 9 / 60.0 + 59 / 3600.0) * 30);
        QCOMPARE(clock.secondAngle(), 354.0);
        QVERIFY(clock.accessibleText().contains(clock.dateText()));
        clock.setShowPeriod(false);
        QCOMPARE(clock.displayText(),
                 clock.hourText() + ':' + clock.minuteText() + ':' + clock.secondText());
        if (! clock.is24Hour()) QVERIFY(clock.accessibleText().contains(clock.periodText()));
        const auto previous = clock.dateTime();
        clock.setTimeZoneId("Invalid/Zone");
        QCOMPARE(clock.timeZoneId(), QByteArray("UTC"));
        QCOMPARE(clock.dateTime(), previous);
    }

    void localAndUtc() {
        const auto  instant = utc("2026-12-31T23:59:59Z");
        ClockSource source;
        source.setMode(ClockSource::Fixed);
        source.setFixedDateTime(instant);
        TestClock clock;
        clock.setSource(&source);
        clock.componentComplete();
        const auto verifyTime = [&](const QDateTime& expected) {
            QCOMPARE(clock.dateTime().toMSecsSinceEpoch(), instant.toMSecsSinceEpoch());
            QCOMPARE(clock.dateTime().date(), expected.date());
            QCOMPARE(clock.dateTime().time(), expected.time());
            QCOMPARE(clock.dateTime().offsetFromUtc(), expected.offsetFromUtc());
        };
        QVERIFY(clock.timeZoneId().isEmpty());
        verifyTime(instant.toLocalTime());
        QSignalSpy zones(&clock, &Clock::timeZoneIdChanged);
        clock.setTimeZoneId("UTC");
        QCOMPARE(clock.timeZoneId(), QByteArray("UTC"));
        QCOMPARE(zones.size(), 1);
        verifyTime(instant.toUTC());
        clock.setTimeZoneId("UTC");
        QCOMPARE(zones.size(), 1);
        clock.setTimeZoneId("Invalid/Zone");
        QCOMPARE(clock.timeZoneId(), QByteArray("UTC"));
        QCOMPARE(zones.size(), 1);
        verifyTime(instant.toUTC());
#if ! QT_CONFIG(timezone)
        clock.setTimeZoneId("Asia/Shanghai");
        QCOMPARE(clock.timeZoneId(), QByteArray("UTC"));
        QCOMPARE(zones.size(), 1);
        verifyTime(instant.toUTC());
#endif
        clock.setTimeZoneId({});
        QVERIFY(clock.timeZoneId().isEmpty());
        QCOMPARE(zones.size(), 2);
        verifyTime(instant.toLocalTime());
        clock.setTimeZoneId({});
        QCOMPARE(zones.size(), 2);
    }

    void zonesAndDst() {
        ClockSource source;
        source.setMode(ClockSource::Fixed);
        source.setFixedDateTime(utc("2026-12-31T23:59:59Z"));
        TestClock clock;
        clock.setSource(&source);
        clock.setTimeZoneId("Asia/Shanghai");
        if (clock.timeZoneId().isEmpty()) QSKIP("Asia/Shanghai unavailable");
        clock.componentComplete();
        QCOMPARE(clock.dateTime().date(), QDate(2027, 1, 1));
        QCOMPARE(clock.dayText(), clock.locale().zeroDigit() + clock.locale().toString(1));
        QCOMPARE(clock.monthText(), clock.locale().zeroDigit() + clock.locale().toString(1));
        QCOMPARE(clock.compactDateText(),
                 clock.locale().toString(clock.dateTime().date(), "ddd, MMM d"));
        QCOMPARE(clock.hour(), 7);
        clock.setTimeZoneId("America/New_York");
        if (clock.timeZoneId() != "America/New_York") QSKIP("America/New_York unavailable");
        QCOMPARE(clock.dayText(), clock.locale().toString(31));
        QCOMPARE(clock.monthText(), clock.locale().toString(12));
        const auto check = [&](const char* instant, int hour, int minute, int offset) {
            source.setFixedDateTime(utc(instant));
            // An inactive view resynchronizes when explicitly rebound.
            clock.resetSource();
            clock.setSource(&source);
            QCOMPARE(clock.hour(), hour);
            QCOMPARE(clock.minute(), minute);
            QCOMPARE(clock.dateTime().offsetFromUtc(), offset);
        };
        check("2026-03-08T06:59:59Z", 1, 59, -18000);
        check("2026-03-08T07:00:00Z", 3, 0, -14400);
        check("2026-11-01T05:30:00Z", 1, 30, -14400);
        check("2026-11-01T06:30:00Z", 1, 30, -18000);
    }

    void activeLifecycle() {
        ClockSource source;
        source.setMode(ClockSource::Fixed);
        source.setFixedDateTime(utc("2026-10-16T10:09:59Z"));
        QQuickWindow window;
        window.resize(300, 300);
        QQuickItem viewport(window.contentItem());
        viewport.setSize({ 200, 100 });
        viewport.setClip(true);
        TestClock clock(&viewport);
        clock.setSize({ 80, 80 });
        clock.setY(150);
        clock.setSource(&source);
        clock.setTimeZoneId("UTC");
        clock.setShowSeconds(true);
        clock.componentComplete();
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(! clock.active());
        QSignalSpy changes(&clock, &Clock::changed);
        source.setFixedDateTime(utc("2026-10-16T10:10:00Z"));
        QCOMPARE(changes.size(), 0);
        viewport.setHeight(240);
        QVERIFY(clock.active());
        QCOMPARE(clock.minute(), 10);
        QCOMPARE(clock.secondAngle(), 0.0);
        clock.setEnabled(false);
        QVERIFY(clock.active());
        source.setFixedDateTime(utc("2026-10-16T10:11:00Z"));
        QCOMPARE(clock.minute(), 11);
        clock.setVisible(false);
        QVERIFY(! clock.active());
        clock.setVisible(true);
        QVERIFY(clock.active());
        viewport.setOpacity(0);
        QVERIFY(! clock.active());
        viewport.setOpacity(1);
        QVERIFY(clock.active());
        clock.setX(400);
        QVERIFY(! clock.active());
        clock.setX(0);
        QVERIFY(clock.active());
        window.setVisibility(QWindow::Minimized);
        QVERIFY(! clock.active());
        window.showNormal();
        QTRY_VERIFY(clock.active());
        window.hide();
        QVERIFY(! clock.active());
        source.setFixedDateTime(utc("2026-10-16T10:12:00Z"));
        window.show();
        QTRY_VERIFY(clock.active());
        QCOMPARE(clock.minute(), 12);
        clock.setParentItem(nullptr);
        QVERIFY(! clock.active());
    }

    void sourceReplacementAndScope() {
        QQmlEngine engine;
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData("import Qcm.Material as MD\nMD.Clock { width: 200; height: 200 }",
                          QUrl());
        std::unique_ptr<QObject> a(component.create()), b(component.create());
        QVERIFY2(a && b, qPrintable(component.errorString()));
        auto* clock = qobject_cast<Clock*>(a.get());
        QCOMPARE(clock->source(), qobject_cast<Clock*>(b.get())->source());
        QCOMPARE(clock->source()->parent(), &engine);
        auto custom = std::make_unique<ClockSource>();
        custom->setMode(ClockSource::Fixed);
        custom->setFixedDateTime(utc("2026-10-16T10:09:59Z"));
        clock->setTimeZoneId("UTC");
        clock->setSource(custom.get());
        QCOMPARE(clock->hour(), 10);
        custom.reset();
        QCOMPARE(clock->source(), qobject_cast<Clock*>(b.get())->source());
        QPointer<ClockSource> source = clock->source();
        a.reset();
        b.reset();
        QVERIFY(source);
        QVERIFY(! source->findChild<QTimer*>()->isActive());
    }

    void mixedPrecisionAndDisposal() {
        SampledSource source;
        QQuickWindow  window;
        window.resize(400, 200);
        TestClock minute(window.contentItem()), second(window.contentItem());
        minute.setSize({ 100, 100 });
        second.setSize({ 100, 100 });
        minute.setSource(&source);
        second.setSource(&source);
        second.setShowSeconds(true);
        minute.componentComplete();
        second.componentComplete();
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(minute.active() && second.active());
        auto* timer = source.findChild<QTimer*>();
        QCOMPARE(source.findChildren<QTimer*>().size(), 1);
        QCOMPARE(timer->interval(), 1000);
        QSignalSpy minuteChanges(&minute, &Clock::changed), secondChanges(&second, &Clock::changed);
        source.now = source.now.addSecs(1);
        source.sample();
        QCOMPARE(minuteChanges.size(), 0);
        QCOMPARE(secondChanges.size(), 1);
        second.setVisible(false);
        QCOMPARE(timer->interval(), 59000);
        source.now = source.now.addSecs(59);
        source.sample();
        QCOMPARE(minuteChanges.size(), 1);
        QCOMPARE(secondChanges.size(), 1);
        minute.setVisible(false);
        QVERIFY(! timer->isActive());
        source.now = source.now.addDays(1);
        second.setVisible(true);
        QCOMPARE(second.dateTime().toUTC(), source.now);
        QVERIFY(timer->isActive());
        second.setParentItem(nullptr);
        QVERIFY(! timer->isActive());

        auto disposable = std::make_unique<TestClock>(window.contentItem());
        disposable->setSize({ 100, 100 });
        disposable->setSource(&source);
        disposable->componentComplete();
        QVERIFY(timer->isActive());
        disposable.reset();
        QVERIFY(! timer->isActive());
    }

    void deletedEngine() {
        auto engine = std::make_unique<QQmlEngine>();
        engine->addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        auto component = std::make_unique<QQmlComponent>(engine.get());
        component->setData("import Qcm.Material as MD\nMD.Clock {}", QUrl());
        std::unique_ptr<QObject> object(component->create());
        QVERIFY2(object, qPrintable(component->errorString()));
        auto*                 clock  = qobject_cast<Clock*>(object.get());
        QPointer<ClockSource> source = clock->source();
        component.reset();
        engine.reset();
        QVERIFY(! source);
        QVERIFY(! clock->source());
        object.reset();
    }

    void logicalDeletion() {
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(240, 240);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            Item {
                id: root
                width: 240; height: 240
                property MD.ClockBase clock
                MD.ClockSource {
                    id: fixedSource; objectName: "fixedSource"
                    mode: MD.ClockSource.Fixed
                    fixedDateTime: new Date(Date.UTC(2026, 9, 16, 10, 9, 30))
                }
                Component {
                    id: factory
                    MD.Clock { width: 200; height: 200; source: fixedSource }
                }
                Component.onCompleted: clock = factory.createObject(root)
                function retire() { clock.destroy(); }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* root = qobject_cast<QQuickItem*>(object.get());
        root->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QPointer<Clock> clock  = root->findChild<Clock*>();
        auto*           source = root->findChild<ClockSource*>("fixedSource");
        QVERIFY(clock && source && clock->active());
        QSignalSpy changes(clock, &Clock::changed);
        QSignalSpy activityChanges(clock, &Clock::activeChanged);
        QVERIFY(QMetaObject::invokeMethod(root, "retire"));
        // destroy() only posts deletion; exercise the engine's later logical-deletion boundary.
        QQmlData::markAsDeleted(clock);
        QVERIFY(QQmlData::wasDeleted(clock));
        source->setFixedDateTime(source->dateTime().addSecs(60));
        QCOMPARE(changes.size(), 0);
        QCOMPARE(activityChanges.size(), 0);
        QTRY_VERIFY(! clock);
    }

    void presets_data() {
        QTest::addColumn<int>("preset");
        QTest::addColumn<int>("width");
        for (int preset = Clock::Analog; preset <= Clock::StackedDigital; ++preset) {
            QTest::newRow(qPrintable(QString("%1-narrow").arg(preset))) << preset << 160;
            QTest::newRow(qPrintable(QString("%1-wide").arg(preset))) << preset << 400;
        }
    }

    void presets() {
        QFETCH(int, preset);
        QFETCH(int, width);
        QQmlEngine   engine;
        QQuickWindow window;
        window.resize(width, 500);
        engine.addImportPath(QStringLiteral(QM_QML_IMPORT_PATH));
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import Qcm.Material as MD
            MD.Clock {
                showDate: true; showSeconds: true; locale: Qt.locale("ar_EG")
                timeZoneId: "UTC"; label: "Local time"
                source: MD.ClockSource {
                    mode: MD.ClockSource.Fixed
                    fixedDateTime: new Date(Date.UTC(2026, 9, 16, 10, 9, 59))
                }
            }
        )",
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* clock = qobject_cast<Clock*>(object.get());
        clock->setParentItem(window.contentItem());
        clock->setPreset(static_cast<Clock::Preset>(preset));
        clock->setWidth(width);
        clock->setHeight(clock->implicitHeight());
        polish(clock);
        QCoreApplication::processEvents();
        QVERIFY(clock->implicitHeight() > 0);
        QVERIFY(clock->implicitWidth() > 0);
        QCOMPARE(clock->hour(), 10);
        const auto ticks = clock->ticks();
        QCOMPARE(ticks.size(), 60);
        QCOMPARE(ticks.first().toMap().value("x").toReal(), .5);
        QVERIFY(std::abs(ticks.first().toMap().value("y").toReal() - .08) < .00001);
        QVERIFY(! clock->active());
        QVERIFY(! clock->source()->findChild<QTimer*>()->isActive());
        clock->setProperty("shape", int(MaterialShape::Flower));
        clock->setShowSeconds(false);
        QCOMPARE(clock->hourAngle(), (10 + 9 / 60.0) * 30);
        QCOMPARE(clock->minuteAngle(), 54.0);
    }
};

int run_clock_presets(int argc, char** argv) {
    ClockPresetsTest test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "clock_presets.moc"
