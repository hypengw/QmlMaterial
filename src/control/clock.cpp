#include "qml_material/control/clock.hpp"
#include "qml_material/core/item_activity.hpp"
#include "qml_material/util/time_format.hpp"
#include <QQmlEngine>
#include <QTimeZone>
#include <private/qqmldata_p.h>
#include <cmath>
#include <numbers>

namespace qml_material
{
struct Clock::Private {
    QPointer<ClockSource>                      source;
    QPointer<ClockSource>                      defaultSource;
    bool                                       defaultResolved = false;
    QMetaObject::Connection                    destroyed;
    std::unique_ptr<ClockSource::Subscription> subscription;
    std::unique_ptr<ItemActivity>              activity;
    QByteArray                                 zone;
    TimeState::HourFormat                      format = TimeState::LocaleFormat;
    Preset                                     preset = Analog;
    bool seconds = false, date = false, period = true, complete = false, active = false;
    struct Snapshot {
        QDateTime time;
        bool      is24 = true;
        QString   hour, minute, second, period, display, date, day, month, weekday, compactDate,
            accessible;
        qreal hourAngle = 0, minuteAngle = 0, secondAngle = 0;
        bool  operator==(const Snapshot&) const = default;
    } snapshot;
};
Clock::Clock(QQuickItem* parent): Control(parent), d(std::make_unique<Private>()) {
    d->activity = std::make_unique<ItemActivity>(this);
    connect(d->activity.get(), &ItemActivity::changed, this, &Clock::updateActivity);
    connect(this, &Control::localeChanged, this, &Clock::refreshSnapshot);
}
Clock::~Clock() {
    disconnect(d->destroyed);
    d->subscription.reset();
    d->activity.reset();
}
void Clock::componentComplete() {
    QPointer<Clock> guard(this);
    Control::componentComplete();
    if (! guard) return;
    d->complete = true;
    bindSource();
}
ClockSource* Clock::source() const {
    if (d->source) return d->source;
    if (! d->defaultResolved) {
        d->defaultSource   = ClockSource::shared(qmlEngine(this));
        d->defaultResolved = true;
    }
    return d->defaultSource;
}
void Clock::setSource(ClockSource* value) {
    if (d->source == value) return;
    d->source = value;
    QPointer<Clock> guard(this);
    bindSource();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT sourceChanged();
}
void Clock::resetSource() { setSource(nullptr); }
void Clock::bindSource() {
    disconnect(d->destroyed);
    d->subscription.reset();
    if (! d->complete || QQmlData::wasDeleted(this)) return;
    auto* s = source();
    if (! s) return;
    d->destroyed    = connect(s, &QObject::destroyed, this, [this, custom = bool(d->source)] {
        if (QQmlData::wasDeleted(this)) return;
        QPointer<Clock> guard(this);
        if (custom)
            bindSource();
        else
            d->subscription.reset();
        if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT sourceChanged();
    });
    d->subscription = s->subscribe(this, showSeconds(), [this](const QDateTime& time) {
        updateSnapshot(time);
    });
    QPointer<Clock> guard(this);
    refreshSnapshot();
    if (guard && ! QQmlData::wasDeleted(this)) updateActivity();
}
void Clock::refreshSnapshot() {
    if (d->subscription) d->subscription->refresh();
}
void Clock::updateActivity() {
    if (QQmlData::wasDeleted(this)) return;
    const auto active  = d->complete && d->activity->active();
    const bool changed = d->active != active;
    d->active          = active;
    QPointer<Clock> guard(this);
    if (d->subscription) d->subscription->setActive(active);
    if (guard && ! QQmlData::wasDeleted(this) && changed) Q_EMIT activeChanged();
}
QByteArray Clock::timeZoneId() const { return d->zone; }
void       Clock::setTimeZoneId(const QByteArray& value) {
    if (d->zone == value || (! value.isEmpty() && ! QTimeZone(value).isValid())) return;
    d->zone = value;
    QPointer<Clock> guard(this);
    refreshSnapshot();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT timeZoneIdChanged();
}
TimeState::HourFormat Clock::hourFormat() const { return d->format; }
void                  Clock::setHourFormat(TimeState::HourFormat value) {
    if (value < TimeState::LocaleFormat || value > TimeState::Hour24 || value == d->format) return;
    d->format = value;
    QPointer<Clock> guard(this);
    refreshSnapshot();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT hourFormatChanged();
}
bool Clock::showSeconds() const { return d->seconds; }
void Clock::setShowSeconds(bool value) {
    if (d->seconds == value) return;
    d->seconds = value;
    QPointer<Clock> guard(this);
    if (d->subscription) d->subscription->setSeconds(value);
    if (! guard || QQmlData::wasDeleted(this)) return;
    if (! d->active) refreshSnapshot();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT showSecondsChanged();
}
bool Clock::showDate() const { return d->date; }
void Clock::setShowDate(bool value) {
    if (d->date == value) return;
    d->date = value;
    QPointer<Clock> guard(this);
    refreshSnapshot();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT showDateChanged();
}
bool Clock::showPeriod() const { return d->period; }
void Clock::setShowPeriod(bool value) {
    if (d->period == value) return;
    d->period = value;
    QPointer<Clock> guard(this);
    refreshSnapshot();
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT showPeriodChanged();
}
Clock::Preset Clock::preset() const { return d->preset; }
void          Clock::setPreset(Preset value) {
    if (value < Analog || value > StackedDigital || value == d->preset) return;
    d->preset = value;
    if (! QQmlData::wasDeleted(this)) Q_EMIT presetChanged();
}
bool      Clock::active() const { return d->active; }
QDateTime Clock::dateTime() const { return d->snapshot.time; }
int       Clock::hour() const { return dateTime().time().hour(); }
int       Clock::minute() const { return dateTime().time().minute(); }
int       Clock::second() const { return dateTime().time().second(); }
bool      Clock::is24Hour() const { return d->snapshot.is24; }
QString   Clock::hourText() const { return d->snapshot.hour; }
QString   Clock::minuteText() const { return d->snapshot.minute; }
QString   Clock::secondText() const { return d->snapshot.second; }
QString   Clock::periodText() const { return d->snapshot.period; }
QString   Clock::displayText() const { return d->snapshot.display; }
QString   Clock::dateText() const { return d->snapshot.date; }
QString   Clock::dayText() const { return d->snapshot.day; }
QString   Clock::monthText() const { return d->snapshot.month; }
QString   Clock::weekdayText() const { return d->snapshot.weekday; }
QString   Clock::compactDateText() const { return d->snapshot.compactDate; }
QString   Clock::accessibleText() const { return d->snapshot.accessible; }
qreal     Clock::hourAngle() const { return d->snapshot.hourAngle; }
qreal     Clock::minuteAngle() const { return d->snapshot.minuteAngle; }
qreal     Clock::secondAngle() const { return d->snapshot.secondAngle; }
void      Clock::updateSnapshot(const QDateTime& instant) {
    if (! d->complete || ! instant.isValid() || QQmlData::wasDeleted(this)) return;
    Private::Snapshot next;
    next.time =
        instant.toTimeZone(d->zone.isEmpty() ? QTimeZone::systemTimeZone() : QTimeZone(d->zone));
    const auto time = next.time.time();
    const auto loc  = locale();
    next.is24       = d->format == TimeState::LocaleFormat ? time_format::uses24Hour(loc)
                                                           : d->format == TimeState::Hour24;
    const auto hour = next.is24 ? time.hour() : (time.hour() % 12 ? time.hour() % 12 : 12);
    next.hour       = time_format::number(loc, hour, true);
    next.minute     = time_format::number(loc, time.minute(), true);
    next.second     = time_format::number(loc, time.second(), true);
    next.period     = next.is24 ? QString() : time.hour() < 12 ? loc.amText() : loc.pmText();
    auto pattern    = next.is24 ? QStringLiteral("HH:mm") : QStringLiteral("hh:mm");
    if (showSeconds()) pattern += QStringLiteral(":ss");
    if (! next.is24) pattern += QStringLiteral(" AP");
    next.display     = loc.toString(time, pattern);
    next.date        = loc.toString(next.time.date(), QLocale::LongFormat);
    next.day         = time_format::number(loc, next.time.date().day(), true);
    next.month       = time_format::number(loc, next.time.date().month(), true);
    next.weekday     = loc.standaloneDayName(next.time.date().dayOfWeek(), QLocale::ShortFormat);
    next.compactDate = loc.toString(next.time.date(), QStringLiteral("ddd, MMM d"));
    next.accessible  = showDate() ? next.date + QStringLiteral(", ") + next.display : next.display;
    if (! showPeriod() && ! next.is24) {
        next.display = next.hour + QLatin1Char(':') + next.minute;
        if (showSeconds()) next.display += QLatin1Char(':') + next.second;
    }
    const auto seconds = showSeconds() ? time.second() : 0;
    next.hourAngle     = (time.hour() % 12 + time.minute() / 60.0 + seconds / 3600.0) * 30;
    next.minuteAngle   = (time.minute() + seconds / 60.0) * 6;
    next.secondAngle   = time.second() * 6;
    if (d->snapshot == next) return;
    d->snapshot = std::move(next);
    Q_EMIT changed();
}
QVariantList Clock::ticks() const {
    static const auto geometry = [] {
        QVariantList result;
        for (int i = 0; i < 60; ++i) {
            const auto angle = i * std::numbers::pi / 30;
            result.append(QVariantMap { { QStringLiteral("x"), .5 + .42 * std::sin(angle) },
                                        { QStringLiteral("y"), .5 - .42 * std::cos(angle) },
                                        { QStringLiteral("angle"), i * 6 },
                                        { QStringLiteral("major"), i % 5 == 0 } });
        }
        return result;
    }();
    return geometry;
}
} // namespace qml_material
