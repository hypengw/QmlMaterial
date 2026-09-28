#pragma once

#include <QDateTime>
#include <QValidator>
#include <QtQml/qqmlregistration.h>
#include <optional>
#include "qml_material/export.hpp"

namespace qml_material
{
class QML_MATERIAL_API DateValidator : public QValidator {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(
        QString dateFormat READ dateFormat WRITE setDateFormat NOTIFY dateFormatChanged FINAL)
    Q_PROPERTY(QDateTime minDate READ minDate WRITE setMinDate NOTIFY minDateChanged FINAL)
    Q_PROPERTY(QString inputFormat READ inputFormat NOTIFY inputFormatChanged FINAL)
    Q_PROPERTY(QDateTime maxDate READ maxDate WRITE setMaxDate NOTIFY maxDateChanged FINAL)
public:
    explicit DateValidator(QObject* parent = nullptr);
    QString               inputFormat() const;
    void                  setLocale(const QLocale&);
    QString               dateFormat() const { return m_format; }
    void                  setDateFormat(const QString&);
    QDateTime             minDate() const { return m_min; }
    void                  setMinDate(const QDateTime&);
    QDateTime             maxDate() const { return m_max; }
    void                  setMaxDate(const QDateTime&);
    State                 validate(QString&, int&) const override;
    Q_INVOKABLE QDateTime parse(const QString&) const;
    Q_INVOKABLE QString   formatDate(const QDateTime&) const;
    Q_INVOKABLE bool      sameDay(const QDateTime&, const QDateTime&) const;
    Q_INVOKABLE bool      monthEnabled(int year, int month, const QDateTime& minimum,
                                       const QDateTime& maximum) const;
    Q_INVOKABLE bool      dateEnabled(const QDateTime&, const QDateTime& minimum,
                                      const QDateTime& maximum) const;
    Q_INVOKABLE bool      rangeValid(const QDateTime& start, const QDateTime& end,
                                     const QDateTime& minimum, const QDateTime& maximum) const;
    Q_INVOKABLE bool      insideRange(const QDateTime& date, const QDateTime& start,
                                      const QDateTime& end) const;
    Q_SIGNAL void         localeChanged();
    Q_SIGNAL void         dateFormatChanged();
    Q_SIGNAL void         inputFormatChanged();
    Q_SIGNAL void         minDateChanged();
    Q_SIGNAL void         maxDateChanged();

private:
    QString                        m_format = QStringLiteral("yyyy-MM-dd");
    QDateTime                      m_min, m_max;
    mutable std::optional<QString> m_cachedText;
    mutable QDateTime              m_cachedDate;
};
} // namespace qml_material
