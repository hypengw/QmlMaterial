#include "qml_material/anim/shimmer.hpp"
#include "qml_material/anim/motion_runner.hpp"
#include "qml_material/core/item_activity.hpp"
#include <QElapsedTimer>
#include <QPointer>
#include <QQuickItem>
#include <QSet>
#include <QSignalBlocker>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace qml_material
{
struct Shimmer::Private {
    explicit Private(Shimmer* owner): runner(1, owner), restTimer(owner) {}
    MotionRunner                  runner;
    QTimer                        restTimer;
    QElapsedTimer                 restClock;
    QSet<QQuickItem*>             consumers;
    QPointer<QQuickItem>          boundsItem;
    std::unique_ptr<ItemActivity> boundsActivity;
    QMetaObject::Connection       boundsDestroyed;
    QString                       operation;
    BoundsMode                    mode     = View;
    int                           duration = 800, pauseDuration = 1500;
    qint64                        restElapsed = 0;
    qreal                         angle = 15, bandWidth = 400, minimum = .25, maximum = 1;
    qreal                         phase = 0, notifiedPhase = 0;
    bool running = true, animations = true, resting = false, notifiedAnimating = false;
};

Shimmer::Shimmer(QObject* parent): QObject(parent), d(std::make_unique<Private>(this)) {
    d->restTimer.setSingleShot(true);
    d->restTimer.setTimerType(Qt::PreciseTimer);
    connect(&d->runner, &MotionRunner::changed, this, [this] {
        d->phase = d->runner.sample().value[0];
        if (d->phase >= 1 && d->runner.state() == MotionRunner::Idle) {
            d->resting     = true;
            d->restElapsed = 0;
        }
        publish();
    });
    connect(&d->runner,
            &MotionRunner::ended,
            this,
            [this](const QString& id, MotionRunner::Reason reason) {
                if (id != d->operation || reason != MotionRunner::Finished) return;
                d->operation.clear();
                reconcile();
            });
    connect(&d->restTimer, &QTimer::timeout, this, [this] {
        if (! d->resting || ! d->restClock.isValid()) return;
        d->restClock.invalidate();
        d->restElapsed = 0;
        d->resting     = false;
        d->phase       = 0;
        reconcile();
    });
}
Shimmer::~Shimmer() {
    const QSignalBlocker block(&d->runner);
    d->runner.stop();
    d->restTimer.stop();
    disconnect(d->boundsDestroyed);
}
Shimmer::BoundsMode Shimmer::boundsMode() const { return d->mode; }
QQuickItem*         Shimmer::boundsItem() const { return d->boundsItem; }
bool                Shimmer::running() const { return d->running; }
bool                Shimmer::animationsEnabled() const { return d->animations; }
int                 Shimmer::duration() const { return d->duration; }
int                 Shimmer::pauseDuration() const { return d->pauseDuration; }
qreal               Shimmer::angle() const { return d->angle; }
qreal               Shimmer::bandWidth() const { return d->bandWidth; }
qreal               Shimmer::minimumOpacity() const { return d->minimum; }
qreal               Shimmer::maximumOpacity() const { return d->maximum; }
qreal               Shimmer::phase() const { return d->phase; }
bool Shimmer::animating() const { return d->runner.state() == MotionRunner::Running; }

void Shimmer::publish() {
    const bool phaseChanged     = d->notifiedPhase != d->phase;
    const bool animationChanged = d->notifiedAnimating != animating();
    d->notifiedPhase            = d->phase;
    d->notifiedAnimating        = animating();
    const QPointer<Shimmer> guard(this);
    if (phaseChanged) Q_EMIT this->phaseChanged();
    if (guard && animationChanged) Q_EMIT animatingChanged();
}
void Shimmer::suspendRest() {
    if (d->restClock.isValid()) {
        d->restElapsed =
            std::min<qint64>(d->pauseDuration, d->restElapsed + d->restClock.elapsed());
        d->restClock.invalidate();
    }
    d->restTimer.stop();
}
void Shimmer::resetSweep() {
    const QSignalBlocker block(&d->runner);
    d->operation.clear();
    d->runner.snapTo({ d->phase });
}
void Shimmer::reconcile() {
    {
        const QSignalBlocker block(&d->runner);
        const bool           active =
            d->running && d->animations && d->minimum < d->maximum && ! d->consumers.isEmpty();
        if (! active) {
            d->runner.pause();
            suspendRest();
        } else if (d->resting && ! d->operation.isEmpty()) {
            // The runner's queued completion owns the transition into the rest timer.
        } else {
            if (d->resting && d->restElapsed >= d->pauseDuration) {
                d->resting     = false;
                d->restElapsed = 0;
                d->phase       = 0;
            }
            if (d->resting) {
                if (! d->restTimer.isActive()) {
                    d->restClock.start();
                    d->restTimer.start(int(d->pauseDuration - d->restElapsed));
                }
            } else if (d->runner.state() == MotionRunner::Paused) {
                d->runner.resume();
            } else if (d->runner.state() != MotionRunner::Running) {
                d->runner.snapTo({ d->phase });
                d->operation = d->runner.animateTo(
                    { 1 }, Motion::tween(std::max(1, qRound(d->duration * (1 - d->phase)))));
            }
        }
    }
    publish();
}
void Shimmer::setConsumerActive(QQuickItem* item, bool active) {
    if (d->consumers.contains(item) == active) return;
    if (active)
        d->consumers.insert(item);
    else
        d->consumers.remove(item);
    reconcile();
}
void Shimmer::observeBounds() {
    d->boundsActivity.reset();
    if (d->mode == Item && d->boundsItem) {
        d->boundsActivity = std::make_unique<ItemActivity>(d->boundsItem);
        connect(d->boundsActivity.get(),
                &ItemActivity::refreshed,
                this,
                &Shimmer::configurationChanged);
    }
}
void Shimmer::setBoundsMode(BoundsMode value) {
    if (value < View || value > Item || d->mode == value) return;
    d->mode = value;
    observeBounds();
    Q_EMIT configurationChanged();
}
void Shimmer::setBoundsItem(QQuickItem* value) {
    if (d->boundsItem == value) return;
    disconnect(d->boundsDestroyed);
    d->boundsItem = value;
    if (value)
        d->boundsDestroyed = connect(value, &QObject::destroyed, this, [this] {
            d->boundsActivity.reset();
            Q_EMIT configurationChanged();
        });
    observeBounds();
    Q_EMIT configurationChanged();
}
void Shimmer::setRunning(bool value) {
    if (d->running == value) return;
    d->running = value;
    const QPointer<Shimmer> guard(this);
    reconcile();
    if (guard) Q_EMIT runningChanged();
}
void Shimmer::setAnimationsEnabled(bool value) {
    if (d->animations == value) return;
    d->animations = value;
    const QPointer<Shimmer> guard(this);
    reconcile();
    if (guard) Q_EMIT configurationChanged();
}
void Shimmer::setDuration(int value) {
    if (value <= 0 || d->duration == value) return;
    d->duration = value;
    if (! d->resting) resetSweep();
    const QPointer<Shimmer> guard(this);
    reconcile();
    if (guard) Q_EMIT configurationChanged();
}
void Shimmer::setPauseDuration(int value) {
    if (value < 0 || d->pauseDuration == value) return;
    suspendRest();
    d->restElapsed =
        d->pauseDuration > 0 ? qRound64(qreal(d->restElapsed) / d->pauseDuration * value) : 0;
    d->pauseDuration = value;
    const QPointer<Shimmer> guard(this);
    reconcile();
    if (guard) Q_EMIT configurationChanged();
}
void Shimmer::setAngle(qreal value) {
    if (! std::isfinite(value) || value < 0) return;
    value = std::fmod(value, 360);
    if (d->angle == value) return;
    d->angle = value;
    Q_EMIT configurationChanged();
}
void Shimmer::setBandWidth(qreal value) {
    if (! std::isfinite(value) || value <= 0 || d->bandWidth == value) return;
    d->bandWidth = value;
    Q_EMIT configurationChanged();
}
void Shimmer::setMinimumOpacity(qreal value) {
    if (! std::isfinite(value) || value < 0 || value > d->maximum || value == d->minimum) return;
    d->minimum = value;
    const QPointer<Shimmer> guard(this);
    reconcile();
    if (guard) Q_EMIT configurationChanged();
}
void Shimmer::setMaximumOpacity(qreal value) {
    if (! std::isfinite(value) || value < d->minimum || value > 1 || value == d->maximum) return;
    d->maximum = value;
    const QPointer<Shimmer> guard(this);
    reconcile();
    if (guard) Q_EMIT configurationChanged();
}
void Shimmer::seek(qreal value) {
    if (d->running || ! std::isfinite(value) || value < 0 || value > 1) return;
    suspendRest();
    d->phase       = value;
    d->resting     = value == 1;
    d->restElapsed = 0;
    resetSweep();
    publish();
}
void Shimmer::restart() {
    suspendRest();
    d->resting     = false;
    d->restElapsed = 0;
    d->phase       = 0;
    resetSweep();
    reconcile();
}
} // namespace qml_material
