#include "qml_material/input/time_state.hpp"
#include <QTime>

namespace qml_material
{
namespace
{
QString digits(const QLocale& locale, int value, bool padded) {
    auto result = locale.toString(value);
    if (padded && value < 10) result.prepend(locale.zeroDigit());
    return result;
}
int parseDigits(const QLocale& locale, const QString& text, int minimum, int maximum) {
    // Reject signs, grouping and whitespace; locale digits and ASCII digits are accepted.
    if (text.isEmpty() || text.size() > 2) return -1;
    for (auto ch : text)
        if (! ch.isDigit()) return -1;
    bool      ok    = false;
    const int value = locale.toInt(text, &ok);
    return ok && value >= minimum && value <= maximum ? value : -1;
}
} // namespace
TimeState::TimeState(QObject* parent): QObject(parent) {
    auto data = m_data.value();
    formatInput(data);
    m_data = data;
}
bool TimeState::uses24Hour(const Data& data) {
    if (data.format != LocaleFormat) return data.format == Hour24;
    const auto pattern = data.locale.timeFormat(QLocale::ShortFormat);
    bool       quoted  = false;
    for (qsizetype i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == QLatin1Char('\'')) {
            if (i + 1 < pattern.size() && pattern[i + 1] == QLatin1Char('\''))
                ++i;
            else
                quoted = ! quoted;
        } else if (! quoted && (pattern[i] == QLatin1Char('A') || pattern[i] == QLatin1Char('a'))) {
            return false;
        }
    }
    return true;
}
bool TimeState::is24Hour() const { return uses24Hour(m_data.value()); }
void TimeState::formatInput(Data& data) {
    const int hour   = data.value / 60;
    data.draftHour   = uses24Hour(data) ? hour : (hour % 12 == 0 ? 12 : hour % 12);
    data.draftMinute = data.value % 60;
    data.hourText    = digits(data.locale, data.draftHour, true);
    data.minuteText  = digits(data.locale, data.draftMinute, true);
}
void TimeState::publish(Data data, bool user) {
    if (data == m_data.value()) return;
    const bool          valueChanged = data.value != value();
    QPointer<TimeState> guard(this);
    m_data = data;
    if (! guard) return;
    Q_EMIT changed();
    if (guard && user && valueChanged && m_data.value() == data) Q_EMIT modified(data.value);
}
void TimeState::setValue(int value) {
    if (value < 0 || value >= 1440) return;
    auto data  = m_data.value();
    data.value = value;
    formatInput(data);
    publish(data);
}
bool TimeState::setTime(int hour, int minute) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return false;
    setValue(hour * 60 + minute);
    return true;
}
void TimeState::setHour(int hour) { setTime(hour, minute()); }
void TimeState::setMinute(int minute) { setTime(hour(), minute); }
void TimeState::setLocale(const QLocale& locale) {
    if (this->locale() == locale) return;
    auto data   = m_data.value();
    data.locale = locale;
    formatInput(data);
    publish(data);
}
void TimeState::setHourFormat(HourFormat format) {
    if (format < LocaleFormat || format > Hour24 || format == hourFormat()) return;
    auto data   = m_data.value();
    data.format = format;
    formatInput(data);
    publish(data);
}
void TimeState::setSelection(Selection selection) {
    if (selection != Hours && selection != Minutes) return;
    auto data      = m_data.value();
    data.selection = selection;
    publish(data);
}
void TimeState::setDisplayMode(DisplayMode mode) {
    if ((mode != Clock && mode != Input) || mode == displayMode()) return;
    if (mode == Clock && ! acceptableInput()) return;
    auto data = m_data.value();
    data.mode = mode;
    formatInput(data);
    publish(data);
}
void TimeState::setPeriod(int period) {
    if (period < 0 || period > 1 || period == this->period()) return;
    auto data  = m_data.value();
    data.value = (hour() % 12 + 12 * period) * 60 + minute();
    if (is24Hour()) formatInput(data);
    publish(data, true);
}
void TimeState::edit(const QString& text, bool hourField) {
    auto data = m_data.value();
    if (hourField) {
        data.hourText  = text;
        data.draftHour = parseDigits(data.locale, text, is24Hour() ? 0 : 1, is24Hour() ? 23 : 12);
    } else {
        data.minuteText  = text;
        data.draftMinute = parseDigits(data.locale, text, 0, 59);
    }
    if (data.draftHour >= 0 && data.draftMinute >= 0) {
        const int hour = is24Hour() ? data.draftHour : data.draftHour % 12 + 12 * period();
        data.value     = hour * 60 + data.draftMinute;
    }
    publish(data, true);
}
void TimeState::editHour(const QString& text) { edit(text, true); }
void TimeState::editMinute(const QString& text) { edit(text, false); }
void TimeState::discardInput() { setValue(value()); }
bool TimeState::commitInput() {
    if (! acceptableInput()) return false;
    discardInput();
    return true;
}
void TimeState::selectDial(int selected, bool complete) {
    auto data = m_data.value();
    if (selection() == Hours) {
        if (selected < 0 || selected > (is24Hour() ? 23 : 12)) return;
        const int hour = is24Hour() ? selected : selected % 12 + 12 * period();
        data.value     = hour * 60 + minute();
        if (complete) data.selection = Minutes;
    } else {
        if (selected < 0 || selected > 59) return;
        data.value = hour() * 60 + selected;
    }
    formatInput(data);
    publish(data, true);
}
void TimeState::step(int delta) {
    const int limit    = selection() == Hours ? 24 : 60;
    const int previous = selection() == Hours ? hour() : minute();
    const int next     = ((previous + delta % limit) % limit + limit) % limit;
    auto      data     = m_data.value();
    data.value         = selection() == Hours ? next * 60 + minute() : hour() * 60 + next;
    formatInput(data);
    publish(data, true);
}
QString TimeState::number(int value, bool padded) const { return digits(locale(), value, padded); }
QString TimeState::inputFormat() const {
    return is24Hour() ? QStringLiteral("HH:mm") : QStringLiteral("hh:mm AP");
}
QString TimeState::formatTime(int value) const {
    return value >= 0 && value < 1440
               ? locale().toString(QTime(value / 60, value % 60), inputFormat())
               : QString();
}
int TimeState::parseText(const QString& text) const {
    const auto time = locale().toTime(text.trimmed(), inputFormat());
    return time.isValid() ? time.hour() * 60 + time.minute() : -1;
}
void TimeValidator::setTime(TimeState* time) {
    if (m_time == time) return;
    disconnect(m_changed);
    disconnect(m_destroyed);
    m_time   = time;
    m_cached = false;
    if (time) {
        m_changed   = connect(time, &TimeState::changed, this, [this] {
            m_cached = false;
            Q_EMIT changed();
        });
        m_destroyed = connect(time, &QObject::destroyed, this, [this] {
            m_cached = false;
            QPointer<TimeValidator> guard(this);
            Q_EMIT timeChanged();
            if (guard) Q_EMIT changed();
        });
    }
    QPointer<TimeValidator> guard(this);
    Q_EMIT timeChanged();
    if (guard) Q_EMIT changed();
}
int TimeValidator::parse(const QString& text) const {
    if (! m_cached || m_text != text) {
        m_text   = text;
        m_value  = m_time ? m_time->parseText(text) : -1;
        m_cached = true;
    }
    return m_value;
}
QValidator::State TimeValidator::validate(QString& text, int&) const {
    return parse(text) >= 0 ? Acceptable : Intermediate;
}
} // namespace qml_material
