#include "qml_material/input/date_validator.hpp"
#include <QPointer>

namespace qml_material
{
namespace
{
QDate civilDate(const QDateTime& value) { return value.toLocalTime().date(); }
} // namespace
DateValidator::DateValidator(QObject* parent): QValidator(parent) {
    connect(this, &QValidator::changed, this, [this] {
        m_cachedText.reset();
    });
}
void DateValidator::setLocale(const QLocale& value) {
    if (locale() == value) return;
    m_cachedText.reset();
    QPointer<DateValidator> guard(this);
    QValidator::setLocale(value);
    if (guard) Q_EMIT localeChanged();
    if (guard) Q_EMIT inputFormatChanged();
}
void DateValidator::setDateFormat(const QString& value) {
    if (m_format == value) return;
    m_format = value;
    m_cachedText.reset();
    QPointer<DateValidator> guard(this);
    Q_EMIT dateFormatChanged();
    if (guard) Q_EMIT inputFormatChanged();
    if (guard) Q_EMIT changed();
}
void DateValidator::setMinDate(const QDateTime& value) {
    if (m_min == value) return;
    m_min = value;
    m_cachedText.reset();
    QPointer<DateValidator> guard(this);
    Q_EMIT minDateChanged();
    if (guard) Q_EMIT changed();
}
void DateValidator::setMaxDate(const QDateTime& value) {
    if (m_max == value) return;
    m_max = value;
    m_cachedText.reset();
    QPointer<DateValidator> guard(this);
    Q_EMIT maxDateChanged();
    if (guard) Q_EMIT changed();
}
QString DateValidator::inputFormat() const {
    if (! m_format.isEmpty()) return m_format;
    const auto source = locale().dateFormat(QLocale::ShortFormat);
    QString    result;
    bool       quoted = false;
    for (qsizetype i = 0; i < source.size();) {
        const auto ch = source.at(i);
        if (ch == QLatin1Char('\'')) {
            result += ch;
            ++i;
            if (i < source.size() && source.at(i) == ch) {
                result += source.at(i++);
            } else {
                quoted = ! quoted;
            }
        } else if (! quoted && ch == QLatin1Char('y')) {
            const auto begin = i;
            while (i < source.size() && source.at(i) == ch) ++i;
            const auto count = i - begin;
            result += count == 2 ? QStringLiteral("yyyy") : source.mid(begin, count);
        } else {
            result += ch;
            ++i;
        }
    }
    return result;
}
QDateTime DateValidator::parse(const QString& text) const {
    if (m_cachedText && *m_cachedText == text) return m_cachedDate;
    m_cachedText    = text;
    const auto date = locale().toDate(text.trimmed(), inputFormat()).startOfDay();
    m_cachedDate    = dateEnabled(date, m_min, m_max) ? date : QDateTime();
    return m_cachedDate;
}
QString DateValidator::formatDate(const QDateTime& value) const {
    return locale().toString(civilDate(value), inputFormat());
}
QValidator::State DateValidator::validate(QString& text, int&) const {
    return parse(text).isValid() ? Acceptable : Intermediate;
}
bool DateValidator::sameDay(const QDateTime& a, const QDateTime& b) const {
    return a.isValid() && b.isValid() && civilDate(a) == civilDate(b);
}
bool DateValidator::monthEnabled(int year, int month, const QDateTime& minimum,
                                 const QDateTime& maximum) const {
    const QDate first(year, month + 1, 1);
    const QDate last(year, month + 1, first.daysInMonth());
    return first.isValid() && (! minimum.isValid() || last >= civilDate(minimum)) &&
           (! maximum.isValid() || first <= civilDate(maximum)) &&
           (! minimum.isValid() || ! maximum.isValid() || civilDate(minimum) <= civilDate(maximum));
}
bool DateValidator::dateEnabled(const QDateTime& value, const QDateTime& minimum,
                                const QDateTime& maximum) const {
    const auto day = civilDate(value);
    return day.isValid() && (! minimum.isValid() || day >= civilDate(minimum)) &&
           (! maximum.isValid() || day <= civilDate(maximum));
}
bool DateValidator::rangeValid(const QDateTime& start, const QDateTime& end,
                               const QDateTime& minimum, const QDateTime& maximum) const {
    return dateEnabled(start, minimum, maximum) && dateEnabled(end, minimum, maximum) &&
           civilDate(start) <= civilDate(end);
}
bool DateValidator::insideRange(const QDateTime& value, const QDateTime& start,
                                const QDateTime& end) const {
    return value.isValid() && start.isValid() && end.isValid() &&
           civilDate(value) > civilDate(start) && civilDate(value) < civilDate(end);
}
} // namespace qml_material
