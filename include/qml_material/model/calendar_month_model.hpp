#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QLocale>
#include <qqmlregistration.h>
#include <array>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API CalendarMonthModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(QVariantList weekDays READ weekDays NOTIFY localeChanged FINAL)
public:
    enum Role
    {
        DateRole = Qt::UserRole + 1,
        DayRole,
        TodayRole,
        WeekNumberRole,
        MonthRole,
        YearRole
    };
    Q_ENUM(Role)
    explicit CalendarMonthModel(QObject* parent = nullptr);
    int                    month() const { return m_month; }
    int                    year() const { return m_year; }
    QLocale                locale() const { return m_locale; }
    QVariantList           weekDays() const;
    void                   setMonth(int);
    void                   setYear(int);
    void                   setLocale(const QLocale&);
    int                    rowCount(const QModelIndex& parent = {}) const override;
    QVariant               data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QDateTime  dateAt(int index) const;
    Q_SIGNAL void          monthChanged();
    Q_SIGNAL void          yearChanged();
    Q_SIGNAL void          localeChanged();

private:
    void                      populate();
    int                       m_month;
    int                       m_year;
    QLocale                   m_locale;
    QDate                     m_today;
    std::array<QDate, 42>     m_dates;
    std::array<QDateTime, 42> m_date_times;
};
} // namespace qml_material
