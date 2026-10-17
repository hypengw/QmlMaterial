#include "qml_material/util/time_format.hpp"

namespace qml_material::time_format
{
bool uses24Hour(const QLocale& locale) {
    const auto pattern = locale.timeFormat(QLocale::ShortFormat);
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
QString number(const QLocale& locale, int value, bool padded) {
    auto result = locale.toString(value);
    if (padded && value < 10) result.prepend(locale.zeroDigit());
    return result;
}
} // namespace qml_material::time_format
