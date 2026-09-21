#include "qml_material/model/calendar_month_model.hpp"
#include <QPointer>

namespace qml_material
{
CalendarMonthModel::CalendarMonthModel(QObject* parent): QAbstractListModel(parent) {
    const auto today = QDate::currentDate();
    m_month          = today.month() - 1;
    m_year           = today.year();
    populate();
}
void CalendarMonthModel::populate() {
    const QDate first(m_year, m_month + 1, 1);
    const int   offset = (first.dayOfWeek() - m_locale.firstDayOfWeek() + 7) % 7;
    const auto  start  = first.addDays(offset == 0 ? -7 : -offset);
    m_today            = QDate::currentDate();
    for (int i = 0; i < 42; ++i) {
        m_dates[i] = start.addDays(i);
        // QDate becomes UTC midnight in JavaScript; delegates need the local civil day.
        m_date_times[i] = m_dates[i].startOfDay();
    }
}
void CalendarMonthModel::setMonth(int value) {
    if (value < 0 || value > 11 || m_month == value) return;
    m_month = value;
    populate();
    QPointer<CalendarMonthModel> guard(this);
    Q_EMIT dataChanged(index(0), index(41));
    if (guard) Q_EMIT monthChanged();
}
void CalendarMonthModel::setYear(int value) {
    if (value == 0 || value < -271820 || value > 275759 || m_year == value) return;
    m_year = value;
    populate();
    QPointer<CalendarMonthModel> guard(this);
    Q_EMIT dataChanged(index(0), index(41));
    if (guard) Q_EMIT yearChanged();
}
void CalendarMonthModel::setLocale(const QLocale& value) {
    if (m_locale == value) return;
    m_locale = value;
    populate();
    QPointer<CalendarMonthModel> guard(this);
    Q_EMIT dataChanged(index(0), index(41));
    if (guard) Q_EMIT localeChanged();
}
QVariantList CalendarMonthModel::weekDays() const {
    QVariantList result;
    result.reserve(7);
    for (int i = 0; i < 7; ++i) {
        const int day = (m_locale.firstDayOfWeek() - 1 + i) % 7 + 1;
        result.append(QVariantMap {
            { "day", day % 7 },
            { "longName", m_locale.standaloneDayName(day, QLocale::LongFormat) },
            { "shortName", m_locale.standaloneDayName(day, QLocale::ShortFormat) },
            { "narrowName", m_locale.standaloneDayName(day, QLocale::NarrowFormat) } });
    }
    return result;
}
int CalendarMonthModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 42;
}
QDateTime CalendarMonthModel::dateAt(int row) const {
    return row >= 0 && row < 42 ? m_date_times[row] : QDateTime();
}
QVariant CalendarMonthModel::data(const QModelIndex& index, int role) const {
    if (! index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 ||
        index.row() >= 42)
        return {};
    const auto date = m_dates[index.row()];
    switch (role) {
    case DateRole: return dateAt(index.row());
    case DayRole: return date.day();
    case TodayRole: return date == m_today;
    case WeekNumberRole: return date.weekNumber();
    case MonthRole: return date.month() - 1;
    case YearRole: return date.year();
    default: return {};
    }
}
QHash<int, QByteArray> CalendarMonthModel::roleNames() const {
    return { { DateRole, "date" },   { DayRole, "day" },
             { TodayRole, "today" }, { WeekNumberRole, "weekNumber" },
             { MonthRole, "month" }, { YearRole, "year" } };
}
} // namespace qml_material
