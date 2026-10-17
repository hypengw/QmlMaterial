#pragma once

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>
#include <functional>
#include <memory>
#include "qml_material/export.hpp"

QT_BEGIN_NAMESPACE
class QQmlEngine;
QT_END_NAMESPACE
namespace qml_material
{
/** @ingroup control
 * Absolute UTC time with demand-driven second/minute scheduling.
 */
class QML_MATERIAL_API ClockSource : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(QDateTime fixedDateTime READ fixedDateTime WRITE setFixedDateTime NOTIFY
                   fixedDateTimeChanged FINAL)
    Q_PROPERTY(QDateTime dateTime READ dateTime NOTIFY dateTimeChanged FINAL)
public:
    enum Mode
    {
        System,
        Fixed
    };
    Q_ENUM(Mode)
    class QML_MATERIAL_API Subscription : public QObject {
    public:
        ~Subscription() override;
        void setActive(bool);
        void setSeconds(bool);
        void refresh();

    private:
        friend class ClockSource;
        Subscription(ClockSource*, QObject*, bool, std::function<void(const QDateTime&)>);
        QPointer<ClockSource>                 source;
        QPointer<QObject>                     context;
        std::function<void(const QDateTime&)> callback;
        bool                                  active = false, seconds = false;
        QDateTime                             last;
    };
    explicit ClockSource(QObject* parent = nullptr);
    ~ClockSource() override;
    Mode                          mode() const;
    void                          setMode(Mode);
    QDateTime                     fixedDateTime() const;
    void                          setFixedDateTime(const QDateTime&);
    QDateTime                     dateTime() const;
    Q_INVOKABLE void              refresh();
    std::unique_ptr<Subscription> subscribe(QObject* context, bool seconds,
                                            std::function<void(const QDateTime&)>);
    static ClockSource*           shared(QQmlEngine* engine);
    Q_SIGNAL void                 modeChanged();
    Q_SIGNAL void                 fixedDateTimeChanged();
    Q_SIGNAL void                 dateTimeChanged();

protected:
    virtual QDateTime currentDateTime() const;
    void              sample();

private:
    struct Private;
    std::unique_ptr<Private> d;
    void                     synchronize(bool force, Subscription* only = nullptr);
    void                     schedule();
};
} // namespace qml_material
