#pragma once

#include "qml_material/input/date_validator.hpp"
#include "qml_material/model/calendar_month_model.hpp"
#include "qml_material/input/interactive_item.hpp"
#include <QQmlParserStatus>
Q_MOC_INCLUDE("qml_material/control/text_field.hpp")

namespace qml_material
{
class TextField;
class CalendarDay;
/** @ingroup control */
class QML_MATERIAL_API DatePickerState : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(
        int selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QDateTime selectedDate READ selectedDate WRITE setSelectedDate NOTIFY
                   selectionChanged FINAL)
    Q_PROPERTY(
        QDateTime rangeStart READ rangeStart WRITE setRangeStart NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QDateTime rangeEnd READ rangeEnd WRITE setRangeEnd NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QDateTime minDate READ minDate WRITE setMinDate NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(QDateTime maxDate READ maxDate WRITE setMaxDate NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(int firstYear READ firstYear NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(int lastYear READ lastYear NOTIFY constraintsChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY viewChanged FINAL)
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY viewChanged FINAL)
    Q_PROPERTY(int navigation READ navigation WRITE setNavigation NOTIFY viewChanged FINAL)
    Q_PROPERTY(bool inputMode READ inputMode WRITE setInputMode NOTIFY inputModeChanged FINAL)
    Q_PROPERTY(bool selectionValid READ selectionValid NOTIFY validityChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(DateValidator* rules READ rules CONSTANT FINAL)
    Q_PROPERTY(CalendarMonthModel* calendar READ calendar CONSTANT FINAL)
    Q_PROPERTY(
        TextField* startInput READ startInput WRITE setStartInput NOTIFY editorsChanged FINAL)
    Q_PROPERTY(TextField* endInput READ endInput WRITE setEndInput NOTIFY editorsChanged FINAL)
public:
    explicit DatePickerState(QObject* parent = nullptr);
    ~DatePickerState() override;
    void                classBegin() override { m_complete = false; }
    void                componentComplete() override;
    int                 selectionMode() const { return m_mode; }
    QDateTime           selectedDate() const { return m_selected; }
    QDateTime           rangeStart() const { return m_start; }
    QDateTime           rangeEnd() const { return m_end; }
    QDateTime           minDate() const { return m_rules.minDate(); }
    QDateTime           maxDate() const { return m_rules.maxDate(); }
    int                 firstYear() const;
    int                 lastYear() const;
    int                 year() const { return m_calendar.year(); }
    int                 month() const { return m_calendar.month(); }
    int                 navigation() const { return m_navigation; }
    bool                inputMode() const { return m_inputMode; }
    bool                selectionValid() const;
    QLocale             locale() const { return m_calendar.locale(); }
    DateValidator*      rules() { return &m_rules; }
    CalendarMonthModel* calendar() { return &m_calendar; }
    TextField*          startInput() const;
    TextField*          endInput() const;
    void                setSelectionMode(int);
    void                setSelectedDate(const QDateTime&);
    void                setRangeStart(const QDateTime&);
    void                setRangeEnd(const QDateTime&);
    void                setMinDate(const QDateTime&);
    void                setMaxDate(const QDateTime&);
    void                setYear(int);
    void                setMonth(int);
    void                setNavigation(int);
    void                setInputMode(bool);
    void                setLocale(const QLocale&);
    void                setStartInput(TextField*);
    void                setEndInput(TextField*);
    Q_INVOKABLE void    setRange(const QDateTime& start, const QDateTime& end);
    Q_INVOKABLE void    selectYear(int);
    Q_INVOKABLE void    selectMonth(int);
    Q_INVOKABLE void    shiftMonth(int delta);
    Q_INVOKABLE void    toggleNavigation() { setNavigation(m_navigation ? 0 : 1); }
    Q_INVOKABLE bool    monthEnabled(int year, int month) const;
    Q_INVOKABLE bool    dayEnabled(const QDateTime&) const;
    Q_INVOKABLE bool    selected(const QDateTime&) const;
    Q_INVOKABLE bool    inRange(const QDateTime&) const;
    Q_INVOKABLE void    requestDate(const QDateTime&);
    void                focusDate(const QDateTime&);
    QDateTime           focusedDate() const;
    Q_SIGNAL void       selectionChanged();
    Q_SIGNAL void       validityChanged();
    Q_SIGNAL void       constraintsChanged();
    Q_SIGNAL void       viewChanged();
    Q_SIGNAL void       inputModeChanged();
    Q_SIGNAL void       localeChanged();
    Q_SIGNAL void       editorsChanged();
    Q_SIGNAL void       focusChanged();
    Q_SIGNAL void       selectedDateChangeRequested(const QDateTime& date);
    Q_SIGNAL void       rangeChangeRequested(const QDateTime& start, const QDateTime& end);

private:
    friend class CalendarDay;
    void               syncInputs();
    void               bindEditors();
    void               edit(bool end);
    void               navigateTo(const QDateTime&);
    DateValidator      m_rules;
    CalendarMonthModel m_calendar;
    QDateTime          m_selected, m_start, m_end, m_focused;
    int                m_mode = 0, m_navigation = 0;
    bool    m_inputMode = false, m_editing = false, m_complete = true, m_followSelection = true;
    quint64 m_viewRevision = 0, m_editorRevision = 0;
    QPointer<TextField>            m_startInput, m_endInput;
    QList<QMetaObject::Connection> m_editorConnections;
    QList<QPointer<CalendarDay>>   m_days;
};

/** @ingroup control */
class QML_MATERIAL_API CalendarDay : public InteractiveItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(CalendarDayBase)
    Q_PROPERTY(DatePickerState* dateState READ dateState WRITE setDateState NOTIFY dayChanged FINAL)
    Q_PROPERTY(int dayIndex READ dayIndex WRITE setDayIndex NOTIFY dayChanged FINAL)
    Q_PROPERTY(QDateTime date READ date NOTIFY dayChanged FINAL)
    Q_PROPERTY(bool selected READ selected NOTIFY dayChanged FINAL)
    Q_PROPERTY(bool dayEnabled READ dayEnabled NOTIFY dayChanged FINAL)
    Q_PROPERTY(bool inRange READ inRange NOTIFY dayChanged FINAL)
    Q_PROPERTY(bool tabStop READ tabStop NOTIFY dayChanged FINAL)
public:
    explicit CalendarDay(QQuickItem* parent = nullptr);
    ~CalendarDay() override;
    DatePickerState* dateState() const { return m_state; }
    int              dayIndex() const { return m_index; }
    QDateTime        date() const;
    bool             selected() const;
    bool             dayEnabled() const;
    bool             inRange() const;
    bool             tabStop() const;
    void             setDateState(DatePickerState*);
    void             setDayIndex(int);
    Q_SIGNAL void    dayChanged();

protected:
    void activate() override;
    void keyPressEvent(QKeyEvent*) override;

private:
    QPointer<DatePickerState>      m_state;
    int                            m_index = -1;
    QList<QMetaObject::Connection> m_connections;
};
} // namespace qml_material
