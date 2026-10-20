#include "qml_material/input/date_picker_state.hpp"
#include "qml_material/control/text_field.hpp"
#include "qml_material/util/qt.hpp"
#include <QKeyEvent>
#include <QtQuick/private/qquickitem_p.h>
#include <algorithm>

namespace qml_material
{
DatePickerState::DatePickerState(QObject* parent)
    : QObject(parent),
      m_rules(this),
      m_calendar(this),
      m_selected(QDate::currentDate().startOfDay()) {
    m_rules.setDateFormat({});
    connect(this, &DatePickerState::selectionChanged, this, &DatePickerState::syncInputs);
    connect(this, &DatePickerState::selectionChanged, this, &DatePickerState::validityChanged);
    connect(&m_rules, &DateValidator::inputFormatChanged, this, &DatePickerState::syncInputs);
    connect(&m_rules, &DateValidator::minDateChanged, this, &DatePickerState::constraintsChanged);
    connect(&m_rules, &DateValidator::maxDateChanged, this, &DatePickerState::constraintsChanged);
    connect(this, &DatePickerState::constraintsChanged, this, &DatePickerState::selectionChanged);
    connect(&m_calendar, &CalendarMonthModel::yearChanged, this, &DatePickerState::viewChanged);
    connect(&m_calendar, &CalendarMonthModel::monthChanged, this, &DatePickerState::viewChanged);
}
DatePickerState::~DatePickerState() { utils::disconnectAll(m_editorConnections); }
void DatePickerState::componentComplete() {
    m_complete = true;
    syncInputs();
}
int DatePickerState::firstYear() const {
    return std::max(1, minDate().isValid() ? minDate().toLocalTime().date().year() : 1);
}
int DatePickerState::lastYear() const {
    return std::min(9999, maxDate().isValid() ? maxDate().toLocalTime().date().year() : 9999);
}
TextField* DatePickerState::startInput() const { return m_startInput; }
TextField* DatePickerState::endInput() const { return m_endInput; }
bool       DatePickerState::selectionValid() const {
    if (m_inputMode && ((m_startInput && ! m_startInput->hasAcceptableInput()) ||
                        (m_mode == 1 && m_endInput && ! m_endInput->hasAcceptableInput())))
        return false;
    return m_mode == 0 ? dayEnabled(m_selected)
                       : m_rules.rangeValid(m_start, m_end, minDate(), maxDate());
}
void DatePickerState::setSelectionMode(int value) {
    if ((value != 0 && value != 1) || value == m_mode) return;
    m_mode = value;
    Q_EMIT selectionChanged();
}
void DatePickerState::setSelectedDate(const QDateTime& date) {
    if (m_selected == date) return;
    m_selected = date;
    const QPointer<DatePickerState> guard(this);
    if (m_followSelection) navigateTo(date);
    if (guard) Q_EMIT selectionChanged();
}
void DatePickerState::setRangeStart(const QDateTime& date) { setRange(date, m_end); }
void DatePickerState::setRangeEnd(const QDateTime& date) { setRange(m_start, date); }
void DatePickerState::setRange(const QDateTime& start, const QDateTime& end) {
    if (m_start == start && m_end == end) return;
    m_start = start;
    m_end   = end;
    Q_EMIT selectionChanged();
}
void DatePickerState::setMinDate(const QDateTime& date) { m_rules.setMinDate(date); }
void DatePickerState::setMaxDate(const QDateTime& date) { m_rules.setMaxDate(date); }
void DatePickerState::setYear(int value) {
    if (value < firstYear() || value > lastYear()) return;
    m_followSelection = false;
    ++m_viewRevision;
    m_calendar.setYear(value);
}
void DatePickerState::setMonth(int value) {
    m_followSelection = false;
    ++m_viewRevision;
    m_calendar.setMonth(value);
}
void DatePickerState::navigateTo(const QDateTime& date) {
    if (! date.isValid()) return;
    const auto                      civil    = date.toLocalTime().date();
    const auto                      revision = ++m_viewRevision;
    const QPointer<DatePickerState> guard(this);
    m_calendar.setYear(civil.year());
    if (guard && revision == m_viewRevision) m_calendar.setMonth(civil.month() - 1);
}
void DatePickerState::setNavigation(int value) {
    if (value < 0 || value > 2 || value == m_navigation) return;
    m_navigation = value;
    Q_EMIT viewChanged();
}
void DatePickerState::setInputMode(bool value) {
    if (m_inputMode == value) return;
    m_inputMode = value;
    const QPointer<DatePickerState> guard(this);
    setNavigation(0);
    if (! guard) return;
    navigateTo(m_mode == 0 ? m_selected : m_start);
    if (! guard) return;
    syncInputs();
    if (! guard) return;
    Q_EMIT inputModeChanged();
    if (! guard) return;
    Q_EMIT selectionChanged();
    if (! guard || ! m_complete) return;
    if (value) {
        if (m_startInput) m_startInput->forceActiveFocus(Qt::OtherFocusReason);
    } else
        focusDate(m_mode == 0 ? m_selected : m_start);
}
void DatePickerState::setLocale(const QLocale& locale) {
    if (m_calendar.locale() == locale && m_rules.locale() == locale) return;
    const QPointer<DatePickerState> guard(this);
    m_calendar.setLocale(locale);
    if (! guard) return;
    m_rules.setLocale(locale);
    if (guard) Q_EMIT localeChanged();
}
bool DatePickerState::monthEnabled(int year, int month) const {
    return year >= firstYear() && year <= lastYear() &&
           m_rules.monthEnabled(year, month, minDate(), maxDate());
}
bool DatePickerState::dayEnabled(const QDateTime& date) const {
    return m_rules.dateEnabled(date, minDate(), maxDate());
}
bool DatePickerState::selected(const QDateTime& date) const {
    return m_mode == 0 ? m_rules.sameDay(date, m_selected)
                       : m_rules.sameDay(date, m_start) || m_rules.sameDay(date, m_end);
}
bool DatePickerState::inRange(const QDateTime& date) const {
    return m_mode == 1 && m_rules.insideRange(date, m_start, m_end);
}
void DatePickerState::requestDate(const QDateTime& date) {
    if (! dayEnabled(date)) return;
    if (m_mode == 0)
        Q_EMIT selectedDateChangeRequested(date);
    else if (! m_start.isValid() || m_end.isValid() ||
             ! m_rules.rangeValid(m_start, date, minDate(), maxDate()))
        Q_EMIT rangeChangeRequested(date, QDateTime());
    else
        Q_EMIT rangeChangeRequested(m_start, date);
}
void DatePickerState::selectYear(int year) {
    const QPointer<DatePickerState> guard(this);
    setYear(year);
    if (guard) setNavigation(2);
}
void DatePickerState::selectMonth(int month) {
    if (! monthEnabled(year(), month)) return;
    const QPointer<DatePickerState> guard(this);
    setMonth(month);
    if (guard) setNavigation(0);
}
void DatePickerState::shiftMonth(int delta) {
    const auto shifted = QDate(year(), month() + 1, 1).addMonths(delta);
    if (! monthEnabled(shifted.year(), shifted.month() - 1)) return;
    m_followSelection = false;
    navigateTo(shifted.startOfDay());
}
void DatePickerState::setStartInput(TextField* field) {
    if (m_startInput == field) return;
    m_startInput = field;
    bindEditors();
}
void DatePickerState::setEndInput(TextField* field) {
    if (m_endInput == field) return;
    m_endInput = field;
    bindEditors();
}
void DatePickerState::bindEditors() {
    utils::disconnectAll(m_editorConnections);
    for (bool end : { false, true }) {
        auto* field = end ? m_endInput.data() : m_startInput.data();
        if (! field) continue;
        m_editorConnections.append(connect(field, &QQuickTextInput::textEdited, this, [this, end] {
            edit(end);
        }));
        m_editorConnections.append(
            connect(field, &QQuickTextInput::editingFinished, this, &DatePickerState::syncInputs));
        m_editorConnections.append(connect(field,
                                           &QQuickTextInput::acceptableInputChanged,
                                           this,
                                           &DatePickerState::validityChanged));
        m_editorConnections.append(
            connect(field, &QObject::destroyed, this, &DatePickerState::editorsChanged));
    }
    const QPointer<DatePickerState> guard(this);
    syncInputs();
    if (guard) Q_EMIT editorsChanged();
}
void DatePickerState::edit(bool end) {
    const auto field = end ? m_endInput : m_startInput;
    if (! field) return;
    const auto date = m_rules.parse(field->text());
    m_editing       = true;
    const QPointer<DatePickerState> guard(this);
    if (m_mode == 0)
        Q_EMIT selectedDateChangeRequested(date);
    else
        Q_EMIT rangeChangeRequested(end ? m_start : date, end ? date : m_end);
    if (! guard) return;
    m_editing = false;
    Q_EMIT validityChanged();
}
void DatePickerState::syncInputs() {
    const auto revision = ++m_editorRevision;
    if (! m_complete || m_editing) return;
    const QPointer<DatePickerState> guard(this);
    const auto                      start = m_rules.formatDate(m_mode == 0 ? m_selected : m_start);
    const auto                      end   = m_rules.formatDate(m_end);
    if (m_startInput && m_startInput->text() != start) m_startInput->setText(start);
    if (guard && revision == m_editorRevision && m_endInput && m_endInput->text() != end)
        m_endInput->setText(end);
}
QDateTime DatePickerState::focusedDate() const {
    for (const auto& date : { m_focused, m_mode == 0 ? m_selected : m_start }) {
        const auto civil = date.toLocalTime().date();
        if (civil.year() == year() && civil.month() == month() + 1) return date;
    }
    return m_calendar.firstDate();
}
void DatePickerState::focusDate(const QDateTime& date) {
    if (! date.isValid()) return;
    const QPointer<DatePickerState> guard(this);
    navigateTo(date);
    if (! guard) return;
    m_focused = date;
    Q_EMIT focusChanged();
    if (! guard) return;
    const auto days = m_days;
    for (auto day : days) {
        if (! day || ! m_rules.sameDay(day->date(), date)) continue;
        day->forceActiveFocus(Qt::TabFocusReason);
        return;
    }
}
CalendarDay::CalendarDay(QQuickItem* parent): InteractiveItem(parent) {
    inputHost().acceptsInput = [this] {
        return dayEnabled();
    };
}
CalendarDay::~CalendarDay() {
    utils::disconnectAll(m_connections);
    if (m_state) m_state->m_days.removeAll(this);
}
QDateTime CalendarDay::date() const {
    return m_state ? m_state->calendar()->dateAt(m_index) : QDateTime();
}
bool CalendarDay::selected() const { return m_state && m_state->selected(date()); }
bool CalendarDay::dayEnabled() const { return m_state && m_state->dayEnabled(date()); }
bool CalendarDay::inRange() const { return m_state && m_state->inRange(date()); }
bool CalendarDay::tabStop() const {
    return m_state && m_state->rules()->sameDay(date(), m_state->focusedDate());
}
void CalendarDay::setDayIndex(int index) {
    if (m_index == index) return;
    m_index = index;
    const QPointer<CalendarDay> guard(this);
    cancelActivation();
    if (guard) Q_EMIT dayChanged();
}
void CalendarDay::setDateState(DatePickerState* state) {
    if (m_state == state && (state || m_connections.isEmpty())) return;
    const QPointer<CalendarDay>     guard(this);
    const QPointer<DatePickerState> next(state);
    cancelActivation();
    if (! guard) return;
    utils::disconnectAll(m_connections);
    if (m_state) m_state->m_days.removeAll(this);
    m_state = next;
    if (next) {
        next->m_days.append(this);
        m_connections.append(
            connect(next, &DatePickerState::selectionChanged, this, &CalendarDay::dayChanged));
        m_connections.append(
            connect(next, &DatePickerState::focusChanged, this, &CalendarDay::dayChanged));
        m_connections.append(connect(next, &DatePickerState::constraintsChanged, this, [this] {
            cancelActivation();
        }));
        m_connections.append(
            connect(next->calendar(), &QAbstractItemModel::dataChanged, this, [this] {
                const QPointer<CalendarDay> guard(this);
                cancelActivation();
                if (guard) Q_EMIT dayChanged();
            }));
        m_connections.append(connect(next, &QObject::destroyed, this, [this] {
            setDateState(nullptr);
        }));
    }
    Q_EMIT dayChanged();
}
void CalendarDay::activate() {
    if (dayEnabled()) m_state->requestDate(date());
}
void CalendarDay::keyPressEvent(QKeyEvent* event) {
    if (! m_state) {
        event->ignore();
        return;
    }
    const auto civil     = date().toLocalTime().date();
    auto       target    = civil;
    const auto direction = QQuickItemPrivate::get(this)->effectiveLayoutMirror ? -1 : 1;
    switch (event->key()) {
    case Qt::Key_Left: target = civil.addDays(-direction); break;
    case Qt::Key_Right: target = civil.addDays(direction); break;
    case Qt::Key_Up: target = civil.addDays(-7); break;
    case Qt::Key_Down: target = civil.addDays(7); break;
    case Qt::Key_PageUp: target = civil.addMonths(-1); break;
    case Qt::Key_PageDown: target = civil.addMonths(1); break;
    case Qt::Key_Home: target = QDate(civil.year(), civil.month(), 1); break;
    case Qt::Key_End: target = QDate(civil.year(), civil.month(), civil.daysInMonth()); break;
    default: InteractiveItem::keyPressEvent(event); return;
    }
    event->accept();
    if (isEnabled()) m_state->focusDate(target.startOfDay());
}
} // namespace qml_material
