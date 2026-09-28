#pragma once

#include <QLocale>
#include <QProperty>
#include <QValidator>
#include <QPointer>
#include <QtQml/qqmlregistration.h>
#include "qml_material/export.hpp"

namespace qml_material
{
/** Minute-precision civil time, independent of date and time zone. */
class QML_MATERIAL_API TimeState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY changed FINAL)
    Q_PROPERTY(int hour READ hour WRITE setHour NOTIFY changed FINAL)
    Q_PROPERTY(int minute READ minute WRITE setMinute NOTIFY changed FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY changed FINAL)
    Q_PROPERTY(HourFormat hourFormat READ hourFormat WRITE setHourFormat NOTIFY changed FINAL)
    Q_PROPERTY(bool is24Hour READ is24Hour NOTIFY changed FINAL)
    Q_PROPERTY(Selection selection READ selection WRITE setSelection NOTIFY changed FINAL)
    Q_PROPERTY(DisplayMode displayMode READ displayMode WRITE setDisplayMode NOTIFY changed FINAL)
    Q_PROPERTY(int period READ period WRITE setPeriod NOTIFY changed FINAL)
    Q_PROPERTY(QString hourText READ hourText NOTIFY changed FINAL)
    Q_PROPERTY(QString minuteText READ minuteText NOTIFY changed FINAL)
    Q_PROPERTY(bool hourAcceptable READ hourAcceptable NOTIFY changed FINAL)
    Q_PROPERTY(bool minuteAcceptable READ minuteAcceptable NOTIFY changed FINAL)
    Q_PROPERTY(bool acceptableInput READ acceptableInput NOTIFY changed FINAL)
    Q_PROPERTY(QString displayText READ displayText NOTIFY changed FINAL)
    Q_PROPERTY(QString inputFormat READ inputFormat NOTIFY changed FINAL)
    Q_PROPERTY(QString amText READ amText NOTIFY changed FINAL)
    Q_PROPERTY(QString pmText READ pmText NOTIFY changed FINAL)
public:
    enum HourFormat
    {
        LocaleFormat,
        Hour12,
        Hour24
    };
    Q_ENUM(HourFormat)
    enum Selection
    {
        Hours,
        Minutes
    };
    Q_ENUM(Selection)
    enum DisplayMode
    {
        Clock,
        Input
    };
    Q_ENUM(DisplayMode)
    explicit TimeState(QObject* parent = nullptr);
    int                 value() const { return m_data.value().value; }
    int                 hour() const { return value() / 60; }
    int                 minute() const { return value() % 60; }
    void                setValue(int);
    void                setHour(int);
    void                setMinute(int);
    QLocale             locale() const { return m_data.value().locale; }
    void                setLocale(const QLocale&);
    HourFormat          hourFormat() const { return m_data.value().format; }
    void                setHourFormat(HourFormat);
    bool                is24Hour() const;
    Selection           selection() const { return m_data.value().selection; }
    void                setSelection(Selection);
    DisplayMode         displayMode() const { return m_data.value().mode; }
    void                setDisplayMode(DisplayMode);
    int                 period() const { return hour() >= 12 ? 1 : 0; }
    void                setPeriod(int);
    QString             hourText() const { return m_data.value().hourText; }
    QString             minuteText() const { return m_data.value().minuteText; }
    bool                hourAcceptable() const { return m_data.value().draftHour >= 0; }
    bool                minuteAcceptable() const { return m_data.value().draftMinute >= 0; }
    bool                acceptableInput() const { return hourAcceptable() && minuteAcceptable(); }
    QString             displayText() const { return formatTime(value()); }
    QString             inputFormat() const;
    QString             amText() const { return locale().amText(); }
    QString             pmText() const { return locale().pmText(); }
    Q_INVOKABLE bool    setTime(int hour, int minute);
    Q_INVOKABLE void    editHour(const QString&);
    Q_INVOKABLE void    editMinute(const QString&);
    Q_INVOKABLE void    discardInput();
    Q_INVOKABLE bool    commitInput();
    Q_INVOKABLE void    selectDial(int value, bool complete = false);
    Q_INVOKABLE void    step(int delta);
    Q_INVOKABLE QString number(int value, bool padded = false) const;
    Q_INVOKABLE QString formatTime(int value) const;
    Q_INVOKABLE int     parseText(const QString&) const;
    Q_SIGNAL void       changed();
    Q_SIGNAL void       modified(int value);

private:
    struct Data {
        int         value = 0;
        QLocale     locale;
        HourFormat  format    = LocaleFormat;
        Selection   selection = Hours;
        DisplayMode mode      = Clock;
        QString     hourText, minuteText;
        int         draftHour = 0, draftMinute = 0;
        bool        operator==(const Data&) const = default;
    };
    static bool     uses24Hour(const Data&);
    static void     formatInput(Data&);
    void            edit(const QString&, bool hour);
    void            publish(Data, bool user = false);
    QProperty<Data> m_data;
};

class QML_MATERIAL_API TimeValidator : public QValidator {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qml_material::TimeState* time READ time WRITE setTime NOTIFY timeChanged FINAL)
public:
    explicit TimeValidator(QObject* parent = nullptr): QValidator(parent) {}
    TimeState*      time() const { return m_time; }
    void            setTime(TimeState*);
    State           validate(QString&, int&) const override;
    Q_INVOKABLE int parse(const QString&) const;
    Q_SIGNAL void   timeChanged();

private:
    QPointer<TimeState>     m_time;
    QMetaObject::Connection m_changed, m_destroyed;
    mutable QString         m_text;
    mutable int             m_value  = -1;
    mutable bool            m_cached = false;
};
} // namespace qml_material
