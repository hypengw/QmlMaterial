#include "qml_material/clock/clock_source.hpp"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QTimer>
#include <private/qqmldata_p.h>

namespace qml_material
{
struct ClockSource::Private {
    Mode                 mode  = System;
    QDateTime            fixed = QDateTime::currentDateTimeUtc();
    QDateTime            time  = fixed;
    QTimer               timer;
    QList<Subscription*> subscriptions;
};
ClockSource::Subscription::Subscription(ClockSource* s, QObject* c, bool secs,
                                        std::function<void(const QDateTime&)> cb)
    : source(s), context(c), callback(std::move(cb)), seconds(secs) {
    connect(c, &QObject::destroyed, this, [this] {
        setActive(false);
    });
}
ClockSource::Subscription::~Subscription() {
    if (source) {
        source->d->subscriptions.removeOne(this);
        source->schedule();
    }
}
void ClockSource::Subscription::setActive(bool value) {
    if (active == value) return;
    active = value;
    if (source) {
        if (active)
            source->synchronize(true, this);
        else
            source->schedule();
    }
}
void ClockSource::Subscription::setSeconds(bool value) {
    if (seconds == value) return;
    seconds = value;
    if (source && active) source->synchronize(true, this);
}
void ClockSource::Subscription::refresh() {
    if (source) source->synchronize(true, this);
}
ClockSource::ClockSource(QObject* parent): QObject(parent), d(std::make_unique<Private>()) {
    d->timer.setSingleShot(true);
    d->timer.setParent(this);
    d->timer.setTimerType(Qt::PreciseTimer);
    connect(&d->timer, &QTimer::timeout, this, &ClockSource::sample);
    if (auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        connect(app,
                &QGuiApplication::applicationStateChanged,
                this,
                [this](Qt::ApplicationState state) {
                    if (state == Qt::ApplicationActive) refresh();
                });
    }
}
ClockSource::~ClockSource() {
    for (auto* subscription : d->subscriptions) subscription->source = nullptr;
}
ClockSource::Mode ClockSource::mode() const { return d->mode; }
QDateTime         ClockSource::dateTime() const { return d->time; }
QDateTime         ClockSource::fixedDateTime() const { return d->fixed; }
QDateTime         ClockSource::currentDateTime() const { return QDateTime::currentDateTimeUtc(); }
void              ClockSource::setMode(Mode value) {
    if ((value != System && value != Fixed) || d->mode == value) return;
    d->mode = value;
    QPointer<ClockSource> guard(this);
    synchronize(true);
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT modeChanged();
}
void ClockSource::setFixedDateTime(const QDateTime& value) {
    if (! value.isValid() || d->fixed == value) return;
    d->fixed = value.toUTC();
    QPointer<ClockSource> guard(this);
    if (d->mode == Fixed) synchronize(true);
    if (guard && ! QQmlData::wasDeleted(this)) Q_EMIT fixedDateTimeChanged();
}
std::unique_ptr<ClockSource::Subscription>
ClockSource::subscribe(QObject* context, bool seconds,
                       std::function<void(const QDateTime&)> callback) {
    if (! context || ! callback) return {};
    auto subscription = std::unique_ptr<Subscription>(
        new Subscription(this, context, seconds, std::move(callback)));
    d->subscriptions.append(subscription.get());
    return subscription;
}
ClockSource* ClockSource::shared(QQmlEngine* engine) {
    QObject* scope = engine ? static_cast<QObject*>(engine) : QCoreApplication::instance();
    if (! scope) return nullptr;
    const auto name   = QStringLiteral("_qm_sharedClockSource");
    auto*      source = scope->findChild<ClockSource*>(name, Qt::FindDirectChildrenOnly);
    if (! source) {
        source = new ClockSource(scope);
        source->setObjectName(name);
        QQmlEngine::setObjectOwnership(source, QQmlEngine::CppOwnership);
    }
    return source;
}
void ClockSource::refresh() { synchronize(true); }
void ClockSource::sample() { synchronize(false); }
void ClockSource::synchronize(bool force, Subscription* only) {
    if (QQmlData::wasDeleted(this)) return;
    const auto next = d->mode == Fixed ? d->fixed : currentDateTime().toUTC();
    if (! next.isValid()) return;
    const bool changed = next != d->time;
    d->time            = next;
    schedule();
    QPointer<ClockSource> guard(this);
    if (changed) Q_EMIT dateTimeChanged();
    if (! guard || QQmlData::wasDeleted(this)) return;
    if (d->time != next) return;
    QList<QPointer<Subscription>> subscribers;
    for (auto* s : d->subscriptions) subscribers.append(s);
    for (auto& s : subscribers) {
        if (! s || (only && s != only) || (! s->active && s != only) || ! s->context ||
            QQmlData::wasDeleted(s->context))
            continue;
        const qint64 period = s->seconds ? 1000 : 60000;
        const auto   bucket = [period](const QDateTime& t) {
            const auto ms = t.toMSecsSinceEpoch();
            return ms - (ms % period + period) % period;
        };
        if (! force && s->last.isValid() && bucket(s->last) == bucket(next)) continue;
        s->last             = next;
        const auto callback = s->callback;
        callback(next);
        if (! guard || QQmlData::wasDeleted(this)) return;
        if (d->time != next) return;
    }
}
void ClockSource::schedule() {
    int period = 0;
    if (d->mode == System && ! QQmlData::wasDeleted(this)) {
        for (auto* s : d->subscriptions) {
            if (! s->active || ! s->context || QQmlData::wasDeleted(s->context)) continue;
            if (s->seconds) {
                period = 1000;
                break;
            }
            period = 60000;
        }
    }
    if (! period) {
        d->timer.stop();
        return;
    }
    const auto ms = currentDateTime().toMSecsSinceEpoch();
    d->timer.start(period - (ms % period + period) % period);
}
} // namespace qml_material
