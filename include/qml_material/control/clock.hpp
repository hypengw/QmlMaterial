#pragma once

#include "qml_material/control/control.hpp"
#include "qml_material/clock/clock_source.hpp"
#include "qml_material/input/time_state.hpp"

namespace qml_material
{
/** @ingroup control */
class QML_MATERIAL_API Clock : public Control {
    Q_OBJECT
    QML_NAMED_ELEMENT(ClockBase)
    Q_PROPERTY(qml_material::ClockSource* source READ source WRITE setSource RESET resetSource
                   NOTIFY sourceChanged FINAL)
    Q_PROPERTY(
        QByteArray timeZoneId READ timeZoneId WRITE setTimeZoneId NOTIFY timeZoneIdChanged FINAL)
    Q_PROPERTY(qml_material::TimeState::HourFormat hourFormat READ hourFormat WRITE setHourFormat
                   NOTIFY hourFormatChanged FINAL)
    Q_PROPERTY(
        bool showSeconds READ showSeconds WRITE setShowSeconds NOTIFY showSecondsChanged FINAL)
    Q_PROPERTY(bool showDate READ showDate WRITE setShowDate NOTIFY showDateChanged FINAL)
    Q_PROPERTY(bool showPeriod READ showPeriod WRITE setShowPeriod NOTIFY showPeriodChanged FINAL)
    Q_PROPERTY(Preset preset READ preset WRITE setPreset NOTIFY presetChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(QDateTime dateTime READ dateTime NOTIFY changed FINAL)
    Q_PROPERTY(int hour READ hour NOTIFY changed FINAL)
    Q_PROPERTY(int minute READ minute NOTIFY changed FINAL)
    Q_PROPERTY(int second READ second NOTIFY changed FINAL)
    Q_PROPERTY(bool is24Hour READ is24Hour NOTIFY changed FINAL)
    Q_PROPERTY(QString hourText READ hourText NOTIFY changed FINAL)
    Q_PROPERTY(QString minuteText READ minuteText NOTIFY changed FINAL)
    Q_PROPERTY(QString secondText READ secondText NOTIFY changed FINAL)
    Q_PROPERTY(QString periodText READ periodText NOTIFY changed FINAL)
    Q_PROPERTY(QString displayText READ displayText NOTIFY changed FINAL)
    Q_PROPERTY(QString dateText READ dateText NOTIFY changed FINAL)
    Q_PROPERTY(QString dayText READ dayText NOTIFY changed FINAL)
    Q_PROPERTY(QString monthText READ monthText NOTIFY changed FINAL)
    Q_PROPERTY(QString weekdayText READ weekdayText NOTIFY changed FINAL)
    Q_PROPERTY(QString compactDateText READ compactDateText NOTIFY changed FINAL)
    Q_PROPERTY(QString accessibleText READ accessibleText NOTIFY changed FINAL)
    Q_PROPERTY(qreal hourAngle READ hourAngle NOTIFY changed FINAL)
    Q_PROPERTY(qreal minuteAngle READ minuteAngle NOTIFY changed FINAL)
    Q_PROPERTY(qreal secondAngle READ secondAngle NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList ticks READ ticks CONSTANT FINAL)
public:
    enum Preset
    {
        Analog,
        ExpressiveAnalog,
        Digital,
        StackedDigital
    };
    Q_ENUM(Preset)
    explicit Clock(QQuickItem* parent = nullptr);
    ~Clock() override;
    ClockSource*          source() const;
    void                  setSource(ClockSource*);
    void                  resetSource();
    QByteArray            timeZoneId() const;
    void                  setTimeZoneId(const QByteArray&);
    TimeState::HourFormat hourFormat() const;
    void                  setHourFormat(TimeState::HourFormat);
    bool                  showSeconds() const;
    void                  setShowSeconds(bool);
    bool                  showDate() const;
    void                  setShowDate(bool);
    bool                  showPeriod() const;
    void                  setShowPeriod(bool);
    Preset                preset() const;
    void                  setPreset(Preset);
    bool                  active() const;
    QDateTime             dateTime() const;
    int                   hour() const;
    int                   minute() const;
    int                   second() const;
    bool                  is24Hour() const;
    QString               hourText() const;
    QString               minuteText() const;
    QString               secondText() const;
    QString               periodText() const;
    QString               displayText() const;
    QString               dateText() const;
    QString               dayText() const;
    QString               monthText() const;
    QString               weekdayText() const;
    QString               compactDateText() const;
    QString               accessibleText() const;
    qreal                 hourAngle() const;
    qreal                 minuteAngle() const;
    qreal                 secondAngle() const;
    QVariantList          ticks() const;
    Q_SIGNAL void         sourceChanged();
    Q_SIGNAL void         timeZoneIdChanged();
    Q_SIGNAL void         hourFormatChanged();
    Q_SIGNAL void         showSecondsChanged();
    Q_SIGNAL void         showDateChanged();
    Q_SIGNAL void         showPeriodChanged();
    Q_SIGNAL void         presetChanged();
    Q_SIGNAL void         activeChanged();
    Q_SIGNAL void         changed();

protected:
    void componentComplete() override;

private:
    struct Private;
    std::unique_ptr<Private> d;
    void                     bindSource();
    void                     updateActivity();
    void                     refreshSnapshot();
    void                     updateSnapshot(const QDateTime&);
};
} // namespace qml_material
