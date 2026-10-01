#include "qml_material/control/progress_indicator.hpp"
#include "qml_material/anim/linear_indicator_updator.hpp"
#include "qml_material/anim/circular_indicator_updator.hpp"
#include "qml_material/anim/interpolator.hpp"
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>
#include <QtQuick/QQuickWindow>
#include <algorithm>
#include <cmath>

namespace qml_material
{
namespace
{
struct Transition {
    qreal        value    = 0;
    qreal        from     = 0;
    qreal        target   = 0;
    qreal        elapsed  = 0;
    qreal        duration = 0;
    QEasingCurve easing   = anim::standard();
    bool         active() const { return elapsed < duration; }
    void         set(qreal next, qreal ms, QEasingCurve curve = anim::standard()) {
        if (target == next && ms > 0) return;
        from     = value;
        target   = next;
        elapsed  = 0;
        duration = ms;
        easing   = curve;
        if (ms <= 0) value = next;
    }
    void advance(qreal ms) {
        if (! active()) return;
        elapsed = std::min(duration, elapsed + ms);
        value   = std::lerp(from, target, easing.valueForProgress(elapsed / duration));
    }
};
bool valid(qreal value) { return std::isfinite(value) && value >= 0; }
} // namespace

struct ProgressIndicator::Private {
    explicit Private(bool circle): circular(circle), amplitude(circle ? 1.6 : 3) {}
    bool                           circular;
    bool                           running         = true;
    bool                           runningOverride = false;
    bool                           wavy            = false;
    bool                           animations      = true;
    bool                           active          = false;
    bool                           initialized     = false;
    qreal                          stroke          = 4;
    qreal                          wavelength      = -1;
    qreal                          amplitude;
    int                            cycle       = -1;
    qreal                          gap         = 4;
    bool                           gapOverride = false;
    qreal                          angleGap    = -1;
    bool                           angleReset  = false;
    qreal                          angle       = -90;
    QColor                         color       = Qt::black;
    QColor                         track       = Qt::transparent;
    QColor                         stop        = Qt::black;
    int                            type        = -1;
    CompletionBehavior             completion  = Drain;
    AnimStateType                  state       = Stopped;
    Transition                     display;
    Transition                     amp;
    Transition                     drain;
    Transition                     opacity;
    qreal                          runElapsed  = 0;
    qreal                          endElapsed  = 0;
    qreal                          endDuration = 0;
    qreal                          endFrom     = 0;
    qreal                          phase       = 0;
    LinearIndicatorUpdator*        linear      = nullptr;
    CircularIndicatorUpdator*      circle      = nullptr;
    QPointer<QQuickWindow>         window;
    QMetaObject::Connection        frameConnection;
    QMetaObject::Connection        visibilityConnection;
    QList<QMetaObject::Connection> viewportConnections;
    bool                           inViewport = false;
    QElapsedTimer                  clock;
    struct Properties {
        bool   running = true, wavy = false, animationsEnabled = true;
        qreal  strokeWidth = 4, waveLength = -1, waveAmplitude = -1, gapSize = 4, gapAngle = 8,
               startAngle        = -90;
        int    waveCycleDuration = -1, type = -1;
        QColor color = Qt::black, trackColor = Qt::transparent, stopIndicatorColor = Qt::black;
        CompletionBehavior completionBehavior = Drain;
    } notified;
};

ProgressIndicator::ProgressIndicator(bool circular, QQuickItem* parent)
    : ProgressBar(parent), d(std::make_unique<Private>(circular)) {
    if (circular)
        d->circle = new CircularIndicatorUpdator(this);
    else {
        d->linear = new LinearIndicatorUpdator(this);
        d->linear->setColors({ d->color });
    }
    setIndeterminate(true);
    connect(this, &ProgressBar::positionChanged, this, [this] {
        synchronize();
    });
    connect(this, &ProgressBar::indeterminateChanged, this, [this] {
        synchronize();
    });
    connect(this, &Control::mirroredChanged, this, &ProgressIndicator::publish);
    connect(this, &QQuickItem::visibleChanged, this, &ProgressIndicator::publish);
    connect(this, &QQuickItem::enabledChanged, this, &ProgressIndicator::publish);
    connect(this, &QQuickItem::windowChanged, this, &ProgressIndicator::bindWindow);
    bindWindow(window());
}
ProgressIndicator::~ProgressIndicator() {
    disconnect(d->frameConnection);
    disconnect(d->visibilityConnection);
    for (const auto& connection : d->viewportConnections) disconnect(connection);
}
bool ProgressIndicator::running() const {
    return d->runningOverride ? d->running : indeterminate();
}
bool  ProgressIndicator::wavy() const { return d->wavy; }
bool  ProgressIndicator::animationsEnabled() const { return d->animations; }
bool  ProgressIndicator::animating() const { return d->active; }
qreal ProgressIndicator::strokeWidth() const { return d->stroke; }
qreal ProgressIndicator::waveLength() const {
    return d->wavelength >= 0 ? d->wavelength
           : d->circular      ? 15
           : d->wavy          ? indeterminate() ? 20 : 40
                              : 30;
}
qreal ProgressIndicator::waveAmplitude() const { return d->amplitude; }
int   ProgressIndicator::waveCycleDuration() const {
    return d->cycle >= 0 ? d->cycle : d->wavy ? 1000 : 1200;
}
qreal  ProgressIndicator::gapSize() const { return d->gap; }
qreal  ProgressIndicator::gapAngle() const { return d->angleGap >= 0 ? d->angleGap : 8; }
qreal  ProgressIndicator::startAngle() const { return d->angle; }
QColor ProgressIndicator::color() const { return d->color; }
QColor ProgressIndicator::trackColor() const { return d->track; }
QColor ProgressIndicator::stopIndicatorColor() const { return d->stop; }
int    ProgressIndicator::type() const {
    return d->type >= 0 ? d->type : d->circular && d->wavy ? 1 : 0;
}
ProgressIndicator::AnimStateType      ProgressIndicator::animationState() const { return d->state; }
ProgressIndicator::CompletionBehavior ProgressIndicator::completionBehavior() const {
    return d->completion;
}
qreal ProgressIndicator::displayedPosition() const { return d->display.value; }
qreal ProgressIndicator::phase() const { return d->phase; }
qreal ProgressIndicator::amplitudeFraction() const { return d->amp.value; }
qreal ProgressIndicator::progress() const {
    return d->circular ? d->circle->progress() : d->linear->progress();
}
QSizeF ProgressIndicator::preferredSize() const {
    if (d->circular) return d->wavy ? QSizeF(48, 48) : QSizeF(32, 32);
    return { d->wavy ? 240.0 : 100.0, d->stroke + (d->wavy ? 2 * d->amplitude : 0) };
}

void ProgressIndicator::componentComplete() {
    QPointer<ProgressIndicator> guard(this);
    ProgressBar::componentComplete();
    if (! guard) return;
    d->initialized = true;
    synchronize(true);
}

void ProgressIndicator::synchronize(bool immediate) {
    {
        const QSignalBlocker linearSignals(d->linear);
        const QSignalBlocker circularSignals(d->circle);
        const auto           animate = d->initialized && d->animations && ! immediate;
        const auto           p       = std::clamp(position(), 0.0, 1.0);
        d->display.set(p, animate ? 150 : 0);
        const auto amp = d->wavy && waveLength() > 0 && d->amplitude > 0 &&
                                 (indeterminate() || (p >= 0.1 && p <= 0.9))
                             ? 1.0
                             : 0.0;
        d->amp.set(amp,
                   animate ? 500 : 0,
                   amp > d->amp.value ? anim::standard() : anim::emphasized_accelerate());
        const auto drain = ! indeterminate() && d->completion == Drain && p >= 1 ? 1.0 : 0.0;
        d->drain.set(
            drain, animate ? (d->circular ? 600 : 900) : 0, QEasingCurve(QEasingCurve::OutCubic));
        const auto previousType = d->circular ? int(d->circle->indeterminateAnimationType())
                                              : int(d->linear->indeterminateAnimationType());
        if (previousType != type()) {
            d->runElapsed = 0;
            d->endElapsed = 0;
            if (d->state == Completing) d->state = Stopped;
            if (d->circular) d->circle->updateCompleteEndProgress(0);
        }
        if (d->circular)
            d->circle->setIndeterminateAnimationType(
                static_cast<CircularIndicatorUpdator::IndeterminateAnimationType>(type()));
        else
            d->linear->setIndeterminateAnimationType(
                static_cast<LinearIndicatorUpdator::IndeterminateAnimationType>(type()));
        if (! indeterminate()) {
            d->state      = Stopped;
            d->endElapsed = 0;
        } else if (running()) {
            if (d->state != Running) {
                d->runElapsed = 0;
                d->endElapsed = 0;
                if (d->circular) d->circle->updateCompleteEndProgress(0);
            }
            d->state = Running;
        } else if (d->state == Running && animate) {
            d->state       = Completing;
            d->endElapsed  = 0;
            d->endFrom     = progress();
            d->endDuration = d->circular ? d->circle->completeEndDuration()
                                         : d->linear->duration() * (1 - d->endFrom);
        } else if (! animate)
            d->state = Stopped;
        if (indeterminate() && d->state == Running) {
            if (d->circular)
                d->circle->update(std::fmod(d->runElapsed, d->circle->duration()) /
                                  d->circle->duration());
            else
                d->linear->update(std::fmod(d->runElapsed, d->linear->duration()) /
                                  d->linear->duration());
        }
        if (! d->animations && indeterminate() && running()) {
            if (d->circular) {
                const auto fraction = type() == 0 ? 0.125 : 0.25;
                d->runElapsed       = fraction * d->circle->duration();
                d->circle->update(fraction);
            } else {
                d->runElapsed = 0.75 * d->linear->duration();
                d->linear->update(0.75);
            }
        }
        d->opacity.set(indeterminate() && d->state == Stopped ? 0 : 1,
                       animate ? 100 : 0,
                       QEasingCurve(QEasingCurve::Linear));
    }
    notifyParameters();
}

bool ProgressIndicator::eligible() const {
    return d->initialized && isVisible() && isEnabled() && d->window && d->window->isVisible() &&
           d->window->visibility() != QWindow::Minimized && d->inViewport;
}
bool ProgressIndicator::needsAnimation() const {
    if (! d->animations) return false;
    const auto wave =
        d->wavy && waveLength() > 0 && d->amplitude > 0 && waveCycleDuration() > 0 &&
        (indeterminate() ? d->state != Stopped : d->display.value > 0 && d->display.value < 1) &&
        (d->amp.value > 0 || d->amp.target > 0);
    return d->display.active() || d->amp.active() || d->drain.active() || d->opacity.active() ||
           (indeterminate() && d->state != Stopped) || wave;
}
void ProgressIndicator::bindWindow(QQuickWindow* window) {
    QPointer<ProgressIndicator> guard(this);
    disconnect(d->frameConnection);
    disconnect(d->visibilityConnection);
    d->window = window;
    d->clock.invalidate();
    if (window) {
        d->frameConnection      = connect(window, &QQuickWindow::afterAnimating, this, [this] {
            QPointer<ProgressIndicator> guard(this);
            refreshViewport();
            if (! guard) return;
            if (eligible() && needsAnimation()) polish();
        });
        d->visibilityConnection = connect(window, &QWindow::visibilityChanged, this, [this] {
            publish();
        });
    }
    observeViewport();
    if (! guard) return;
    publish();
}
void ProgressIndicator::observeViewport() {
    for (const auto& connection : d->viewportConnections) disconnect(connection);
    d->viewportConnections.clear();
    for (auto* item = static_cast<QQuickItem*>(this); item; item = item->parentItem()) {
        const auto observe = [this, item](auto signal) {
            d->viewportConnections.push_back(
                connect(item, signal, this, &ProgressIndicator::refreshViewport));
        };
        observe(&QQuickItem::xChanged);
        observe(&QQuickItem::yChanged);
        observe(&QQuickItem::widthChanged);
        observe(&QQuickItem::heightChanged);
        observe(&QQuickItem::rotationChanged);
        observe(&QQuickItem::scaleChanged);
        observe(&QQuickItem::transformOriginChanged);
        observe(&QQuickItem::clipChanged);
        d->viewportConnections.push_back(
            connect(item, &QQuickItem::parentChanged, this, &ProgressIndicator::observeViewport));
    }
    refreshViewport();
}
void ProgressIndicator::refreshViewport() {
    QRectF area;
    if (window()) {
        auto* root = window()->contentItem();
        area       = mapRectToItem(root, QRectF(0, 0, width(), height()))
                         .intersected(QRectF(0, 0, root->width(), root->height()));
        for (auto* item = parentItem(); item && ! area.isEmpty(); item = item->parentItem()) {
            if (item->clip()) area = area.intersected(item->mapRectToItem(root, item->clipRect()));
        }
    }
    const bool inViewport = ! area.isEmpty();
    if (d->inViewport == inViewport) return;
    d->inViewport = inViewport;
    publish();
}
void ProgressIndicator::schedule() {
    const bool active = eligible() && needsAnimation();
    if (! active)
        d->clock.invalidate();
    else {
        if (! d->clock.isValid()) d->clock.start();
        d->window->update();
    }
    d->active = active;
}
void ProgressIndicator::publish() {
    schedule();
    emit frameChanged();
}
void ProgressIndicator::notifyParameters() {
    QPointer<ProgressIndicator> guard(this);
#define QM_NOTIFY_PROPERTY(name)      \
    if (d->notified.name != name()) { \
        d->notified.name = name();    \
        emit name##Changed();         \
        if (! guard) return;          \
    }
    QM_NOTIFY_PROPERTY(running)
    QM_NOTIFY_PROPERTY(wavy)
    QM_NOTIFY_PROPERTY(animationsEnabled)
    QM_NOTIFY_PROPERTY(strokeWidth)
    QM_NOTIFY_PROPERTY(waveLength)
    QM_NOTIFY_PROPERTY(waveAmplitude)
    QM_NOTIFY_PROPERTY(waveCycleDuration)
    QM_NOTIFY_PROPERTY(gapSize)
    QM_NOTIFY_PROPERTY(gapAngle)
    QM_NOTIFY_PROPERTY(startAngle)
    QM_NOTIFY_PROPERTY(color)
    QM_NOTIFY_PROPERTY(trackColor)
    QM_NOTIFY_PROPERTY(stopIndicatorColor)
    QM_NOTIFY_PROPERTY(type)
    QM_NOTIFY_PROPERTY(completionBehavior)
#undef QM_NOTIFY_PROPERTY
    emit parametersChanged();
    if (guard) publish();
}
void ProgressIndicator::updatePolish() {
    if (! eligible() || ! needsAnimation()) {
        if (d->active) publish();
        return;
    }
    const auto elapsed = d->clock.isValid() ? d->clock.nsecsElapsed() / 1000000.0 : 0;
    d->clock.start();
    advanceAnimations(elapsed);
}
void ProgressIndicator::advanceAnimations(qreal ms) {
    if (! d->animations || ! valid(ms)) return;
    {
        const QSignalBlocker linearSignals(d->linear);
        const QSignalBlocker circularSignals(d->circle);
        d->display.advance(ms);
        d->amp.advance(ms);
        d->drain.advance(ms);
        d->opacity.advance(ms);
        if (indeterminate() && d->state == Running) {
            const auto duration = d->circular ? d->circle->duration() : d->linear->duration();
            d->runElapsed       = std::fmod(d->runElapsed + std::fmod(ms, duration), duration);
            const auto fraction = d->runElapsed / duration;
            if (d->circular)
                d->circle->update(fraction);
            else
                d->linear->update(fraction);
        } else if (indeterminate() && d->state == Completing) {
            const auto consumed = std::min(ms, std::max(0.0, d->endDuration - d->endElapsed));
            d->endElapsed       = std::min(d->endDuration, d->endElapsed + ms);
            const auto fraction = d->endDuration > 0 ? d->endElapsed / d->endDuration : 1;
            if (d->circular) {
                d->runElapsed += consumed;
                d->circle->updateCompleteEndProgress(fraction);
                d->circle->update(std::fmod(d->runElapsed, d->circle->duration()) /
                                  d->circle->duration());
            } else
                d->linear->update(std::lerp(d->endFrom, 1.0, fraction));
            if (fraction >= 1) {
                d->state = Stopped;
                d->opacity.set(0, 100, QEasingCurve(QEasingCurve::Linear));
                d->opacity.advance(ms - consumed);
            }
        }
        if (d->wavy && waveLength() > 0 && d->amplitude > 0 && d->amp.value > 0 &&
            waveCycleDuration() > 0 &&
            (indeterminate() ? d->state != Stopped : d->display.value > 0 && d->display.value < 1))
            d->phase = std::fmod(d->phase + ms / waveCycleDuration(), 1.0);
    }
    publish();
}

ProgressIndicatorState ProgressIndicator::renderState() const {
    ProgressIndicatorState s;
    s.circular          = d->circular;
    s.legacyRadius      = d->circular && ! d->wavy;
    s.indeterminate     = indeterminate();
    s.wavy              = d->wavy;
    s.mirrored          = mirrored();
    s.position          = d->display.value;
    s.phase             = d->phase;
    s.amplitudeFraction = d->amp.value;
    s.strokeWidth       = d->stroke;
    s.waveLength        = waveLength();
    s.waveAmplitude     = d->amplitude;
    s.gapSize           = d->gap;
    s.gapAngle          = d->angleGap >= 0 ? d->angleGap
                          : d->circular && ! d->wavy && ! d->gapOverride && ! d->angleReset ? 8.0
                                                                                            : -1.0;
    s.startAngle        = d->circular ? d->angle : 0;
    s.drain             = d->drain.value;
    s.opacity           = d->opacity.value;
    s.trackColor        = d->track;
    s.stopColor         = d->stop;
    if (! indeterminate()) {
        const auto drain = d->circular ? s.drain : std::min(1.0, s.drain / 0.67);
        s.segments.push_back({ 0, d->display.value * (1 - drain), d->color });
    } else if (d->circular) {
        s.rotation = d->circle->rotation();
        s.segments.push_back({ d->circle->startFraction(), d->circle->endFraction(), d->color });
    } else {
        for (auto* data : d->linear->activeIndicators())
            s.segments.push_back({ data->startFraction(),
                                   data->endFraction(),
                                   d->color,
                                   type() == 1 ? d->gap / 2 : 0 });
    }
    return s;
}

void ProgressIndicator::setRunning(bool v) {
    if (d->runningOverride && d->running == v) return;
    d->runningOverride = true;
    d->running         = v;
    synchronize();
}
void ProgressIndicator::resetRunning() {
    d->runningOverride = false;
    synchronize();
}
void ProgressIndicator::setWavy(bool v) {
    if (d->wavy == v) return;
    d->wavy = v;
    synchronize();
}
void ProgressIndicator::setAnimationsEnabled(bool v) {
    if (d->animations == v) return;
    d->animations = v;
    synchronize(! v);
}
void ProgressIndicator::setStrokeWidth(qreal v) {
    if (! valid(v) || d->stroke == v) return;
    d->stroke = v;
    notifyParameters();
}
void ProgressIndicator::setWaveLength(qreal v) {
    if (! valid(v) || d->wavelength == v) return;
    d->wavelength = v;
    synchronize();
}
void ProgressIndicator::resetWaveLength() {
    d->wavelength = -1;
    synchronize();
}
void ProgressIndicator::setWaveAmplitude(qreal v) {
    if (! valid(v) || ! std::isfinite(d->stroke + 2 * v) || d->amplitude == v) return;
    d->amplitude = v;
    synchronize();
}
void ProgressIndicator::setWaveCycleDuration(int v) {
    if (v < 0 || d->cycle == v) return;
    d->cycle = v;
    notifyParameters();
}
void ProgressIndicator::resetWaveCycleDuration() {
    d->cycle = -1;
    notifyParameters();
}
void ProgressIndicator::setGapSize(qreal v) {
    if (! valid(v) || (d->gapOverride && d->gap == v)) return;
    d->gapOverride = true;
    d->gap         = v;
    notifyParameters();
}
void ProgressIndicator::resetGapSize() {
    d->gapOverride = false;
    d->gap         = 4;
    notifyParameters();
}
void ProgressIndicator::setGapAngle(qreal v) {
    if (! valid(v) || d->angleGap == v) return;
    d->angleGap = v;
    notifyParameters();
}
void ProgressIndicator::resetGapAngle() {
    d->angleReset = true;
    d->angleGap   = -1;
    notifyParameters();
}
void ProgressIndicator::setStartAngle(qreal v) {
    if (! std::isfinite(v) || d->angle == v) return;
    d->angle = v;
    notifyParameters();
}
void ProgressIndicator::setColor(QColor v) {
    if (d->color == v) return;
    d->color = v;
    if (d->linear) {
        const QSignalBlocker blocker(d->linear);
        d->linear->setColors({ v });
    }
    notifyParameters();
}
void ProgressIndicator::setTrackColor(QColor v) {
    if (d->track == v) return;
    d->track = v;
    notifyParameters();
}
void ProgressIndicator::setStopIndicatorColor(QColor v) {
    if (d->stop == v) return;
    d->stop = v;
    notifyParameters();
}
void ProgressIndicator::setType(int v) {
    if (v < 0 || v > 1 || d->type == v) return;
    d->type       = v;
    d->runElapsed = 0;
    synchronize(true);
}
void ProgressIndicator::resetType() {
    d->type       = -1;
    d->runElapsed = 0;
    synchronize(true);
}
void ProgressIndicator::setAnimationState(AnimStateType v) {
    if (v == Running)
        setRunning(true);
    else if (v == Completing)
        setRunning(false);
    else if (v == Stopped) {
        d->runningOverride = true;
        d->running         = false;
        d->state           = Stopped;
        synchronize();
    }
}
void ProgressIndicator::setCompletionBehavior(CompletionBehavior v) {
    if ((v != Keep && v != Drain) || d->completion == v) return;
    d->completion = v;
    synchronize();
}
} // namespace qml_material
