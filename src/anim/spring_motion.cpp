#include "qml_material/anim/spring_motion.hpp"
#include "qml_material/math/color.hpp"
#include <QPointer>
#include <algorithm>
#include <cmath>
#include <limits>

namespace qml_material
{
bool token::SpringSpec::valid() const {
    return std::isfinite(stiffness) && stiffness > 0 && std::isfinite(dampingRatio) &&
           dampingRatio > 0 && std::isfinite(dampingRatio * dampingRatio) &&
           std::isfinite(2 * dampingRatio * std::sqrt(stiffness));
}

namespace anim
{
SpringSample sampleSpring(token::SpringSpec spec, qreal from, qreal to, qreal velocity,
                          qreal seconds) {
    if (seconds <= 0) return { from, velocity };
    const qreal w = std::sqrt(spec.stiffness), z = spec.dampingRatio;
    const qreal x = from - to, t = std::max(qreal(0), seconds), r = -z * w;
    if (std::abs(z - 1) < 1e-7) {
        const qreal b = velocity + w * x, e = std::exp(-w * t);
        return { to + (x + b * t) * e, (b - w * (x + b * t)) * e };
    }
    if (z < 1) {
        const qreal d = w * std::sqrt(1 - z * z), b = (velocity - r * x) / d;
        const qreal c = std::cos(d * t), s = std::sin(d * t), e = std::exp(r * t);
        const qreal p = e * (x * c + b * s);
        return { to + p, r * p + e * d * (b * c - x * s) };
    }
    const qreal d    = std::sqrt(z * z - 1);
    const qreal slow = -w / (z + d), fast = -w * (z + d);
    const qreal a = (velocity - fast * x) / (slow - fast), b = x - a;
    const qreal p = a * std::exp(slow * t), q = b * std::exp(fast * t);
    return { to + p + q, slow * p + fast * q };
}

int springDuration(token::SpringSpec spec, qreal from, qreal to, qreal velocity, qreal threshold) {
    if (from == to && velocity == 0) return 0;
    const qreal w = std::sqrt(spec.stiffness), z = spec.dampingRatio, x = from - to;
    qreal       seconds = 0;
    if (z < 1 - 1e-7) {
        const qreal d         = w * std::sqrt(1 - z * z);
        const qreal amplitude = std::hypot(x, (velocity + z * w * x) / d);
        seconds               = std::max(qreal(0), std::log(amplitude / threshold) / (z * w));
    } else {
        qreal extremum = 0;
        if (std::abs(z - 1) < 1e-7) {
            const qreal b = velocity + w * x;
            if (b != 0) extremum = velocity / (w * b);
        } else {
            const qreal d = std::sqrt(z * z - 1), slow = -w / (z + d), fast = -w * (z + d);
            const qreal a = (velocity - fast * x) / (slow - fast), b = x - a;
            const qreal ratio = -b * fast / (a * slow);
            if (ratio > 0) extremum = std::log(ratio) / (slow - fast);
        }
        qreal low          = std::max(qreal(0), extremum);
        auto  displacement = [&](qreal t) {
            return std::abs(sampleSpring(spec, from, to, velocity, t).value - to);
        };
        if (displacement(low) <= threshold) low = 0;
        if (displacement(low) > threshold) {
            qreal high = std::max(low + .001, .01);
            while (displacement(high) > threshold && high < 2e6) high *= 2;
            for (int i = 0; i < 64; ++i) {
                const qreal middle = (low + high) / 2;
                if (displacement(middle) > threshold)
                    low = middle;
                else
                    high = middle;
            }
            seconds = high;
        }
    }
    return int(std::clamp(
        std::ceil(seconds * 1000), qreal(0), qreal(std::numeric_limits<int>::max() - 1)));
}
} // namespace anim

class SpringClock : public QAbstractAnimation {
public:
    explicit SpringClock(SpringMotionBase* owner): QAbstractAnimation(owner), m_owner(owner) {}
    int duration() const override { return length; }
    int length = 0;

private:
    void              updateCurrentTime(int time) override { m_owner->tick(time); }
    SpringMotionBase* m_owner;
};

SpringMotionBase::SpringMotionBase(int dimensions, QObject* parent)
    : QObject(parent), m_tracks(dimensions), m_clock(std::make_unique<SpringClock>(this)) {}
SpringMotionBase::~SpringMotionBase() = default;
void  SpringMotionBase::classBegin() { m_complete = false; }
void  SpringMotionBase::componentComplete() { m_complete = true; }
qreal SpringMotionBase::target(int i) const { return m_tracks[i].target; }
qreal SpringMotionBase::value(int i) const { return m_tracks[i].current.value; }
qreal SpringMotionBase::velocity(int i) const { return m_tracks[i].current.velocity; }
int   SpringMotionBase::elapsed() const { return m_clock->currentTime(); }
qreal SpringMotionBase::displacementThreshold(int) const { return m_threshold; }

void SpringMotionBase::setTargets(const QList<qreal>& targets, bool snap, bool preserveVelocity) {
    if (targets.size() != m_tracks.size() ||
        ! std::all_of(targets.begin(), targets.end(), [](qreal v) {
            return std::isfinite(v);
        }))
        return;
    bool changed = false;
    for (int i = 0; i < targets.size(); ++i) changed |= targets[i] != m_tracks[i].target;
    if (! changed && ! snap) return;
    ++m_revision;
    for (int i = 0; i < targets.size(); ++i) {
        m_tracks[i].target = targets[i];
        if (! preserveVelocity) m_tracks[i].current.velocity = 0;
    }
    const auto                 revision = m_revision;
    QPointer<SpringMotionBase> guard(this);
    if (snap || ! m_enabled || ! m_complete)
        settle(false);
    else
        restart();
    if (guard && revision == m_revision && changed) Q_EMIT targetChanged();
}
void SpringMotionBase::setSpec(token::SpringSpec spec) {
    if (! spec.valid() || spec == m_spec) return;
    m_spec = spec;
    ++m_revision;
    QPointer<SpringMotionBase> guard(this);
    const auto                 revision = m_revision;
    if (m_running) restart();
    if (guard && revision == m_revision) Q_EMIT specChanged();
}
void SpringMotionBase::setVisibilityThreshold(qreal v) {
    if (! std::isfinite(v) || v <= 0 || v == m_threshold) return;
    m_threshold = v;
    ++m_revision;
    const auto                 revision = m_revision;
    QPointer<SpringMotionBase> guard(this);
    if (m_running) restart();
    if (guard && revision == m_revision) Q_EMIT visibilityThresholdChanged();
}
void SpringMotionBase::setEnabled(bool v) {
    if (v == m_enabled) return;
    m_enabled = v;
    ++m_revision;
    const auto                 revision = m_revision;
    QPointer<SpringMotionBase> guard(this);
    if (! v) settle(false);
    if (guard && revision == m_revision) Q_EMIT enabledChanged();
}
void SpringMotionBase::restart() {
    m_clock->stop();
    int duration = 0;
    for (int i = 0; i < m_tracks.size(); ++i) {
        auto& track           = m_tracks[i];
        track.from            = track.current.value;
        track.initialVelocity = track.current.velocity;
        track.duration        = anim::springDuration(
            m_spec, track.from, track.target, track.initialVelocity, displacementThreshold(i));
        duration = std::max(duration, track.duration);
    }
    if (duration == 0) {
        settle(false);
        return;
    }
    const bool wasRunning = m_running;
    m_running             = true;
    m_clock->length       = duration;
    QPointer<SpringMotionBase> guard(this);
    const auto                 revision = m_revision;
    m_clock->start();
    if (guard && revision == m_revision && ! wasRunning) Q_EMIT runningChanged();
}
void SpringMotionBase::tick(int time) {
    if (! m_running) return;
    if (time == 0) return;
    if (time >= m_clock->length) {
        settle(true);
        return;
    }
    bool changed = false;
    for (auto& track : m_tracks) {
        const auto next =
            time >= track.duration
                ? anim::SpringSample { track.target, 0 }
                : anim::sampleSpring(
                      m_spec, track.from, track.target, track.initialVelocity, time / 1000.0);
        changed |= next.value != track.current.value || next.velocity != track.current.velocity;
        track.current = next;
    }
    if (changed) Q_EMIT valueChanged();
}
void SpringMotionBase::settle(bool notifyFinished) {
    const bool wasRunning = m_running;
    m_running             = false;
    m_clock->stop();
    bool changed = false;
    for (auto& track : m_tracks) {
        changed |= track.current.value != track.target || track.current.velocity != 0;
        track.current = { track.target, 0 };
    }
    const auto                 revision = m_revision;
    QPointer<SpringMotionBase> guard(this);
    if (changed) Q_EMIT valueChanged();
    if (! guard || revision != m_revision) return;
    if (wasRunning) Q_EMIT runningChanged();
    if (guard && revision == m_revision && wasRunning && notifyFinished) Q_EMIT finished();
}
void SpringMotionBase::complete() {
    if (! m_running && std::all_of(m_tracks.begin(), m_tracks.end(), [](const Track& track) {
            return track.current.value == track.target && track.current.velocity == 0;
        }))
        return;
    ++m_revision;
    settle(true);
}
void SpringMotionBase::stop() {
    const bool changed = std::any_of(m_tracks.begin(), m_tracks.end(), [](const Track& track) {
        return track.current.velocity != 0;
    });
    if (! m_running && ! changed) return;
    ++m_revision;
    const bool wasRunning = m_running;
    m_running             = false;
    m_clock->stop();
    for (auto& track : m_tracks) track.current.velocity = 0;
    const auto                 revision = m_revision;
    QPointer<SpringMotionBase> guard(this);
    if (changed) Q_EMIT valueChanged();
    if (guard && revision == m_revision && wasRunning) Q_EMIT runningChanged();
}
SpringMotion::SpringMotion(QObject* parent): SpringMotionBase(1, parent) {}
ColorSpringMotion::ColorSpringMotion(QObject* parent): SpringMotionBase(4, parent) {
    setSpec(token::SpringScheme {}.defaultEffects());
}
QColor ColorSpringMotion::targetColor() const {
    return math::fromOklab({ target(1), target(2), target(3) }, target(0));
}
void ColorSpringMotion::setTargetColor(const QColor& color) {
    if (! color.isValid()) return;
    const auto lab = math::toOklab(color);
    setTargets({ color.alphaF(), lab[0], lab[1], lab[2] });
}
QColor ColorSpringMotion::color() const {
    return math::fromOklab({ std::clamp(value(1), qreal(0), qreal(1)),
                             std::clamp(value(2), qreal(-.5), qreal(.5)),
                             std::clamp(value(3), qreal(-.5), qreal(.5)) },
                           std::clamp(value(0), qreal(0), qreal(1)));
}
CornerSpringMotion::CornerSpringMotion(QObject* parent): SpringMotionBase(4, parent) {
    setSpec(token::SpringScheme {}.defaultEffects());
}
qreal CornerSpringMotion::displacementThreshold(int index) const {
    return std::max(qreal(1e-9), std::abs(target(index) - m_start[index]) * visibilityThreshold());
}
CornersGroup CornerSpringMotion::targetCorners() const {
    CornersGroup result;
    for (int i = 0; i < 4; ++i) result[i] = target(i);
    return result;
}
void CornerSpringMotion::setTargetCorners(const CornersGroup& v) {
    for (int i = 0; i < 4; ++i)
        if (! std::isfinite(v[i])) return;
    if (v == targetCorners()) return;
    const bool reversing = v == m_start;
    m_start              = reversing ? targetCorners() : corners();
    setTargets({ v[0], v[1], v[2], v[3] }, false, reversing);
}
CornersGroup CornerSpringMotion::corners() const {
    CornersGroup result;
    for (int i = 0; i < 4; ++i) result[i] = std::max(qreal(0), value(i));
    return result;
}
PressMotion::PressMotion(QObject* parent): SpringMotion(parent) {
    connect(this, &SpringMotion::valueChanged, this, &PressMotion::releaseIfReady);
}
void PressMotion::setPressed(bool v) {
    if (v == m_pressed) return;
    m_pressed   = v;
    m_releasing = m_active && ! v;
    QPointer<PressMotion> guard(this);
    if (m_active && v)
        setTargetValue(1);
    else
        releaseIfReady();
    if (guard) Q_EMIT pressedChanged();
}
void PressMotion::setActive(bool v) {
    if (v == m_active) return;
    m_active    = v;
    m_releasing = false;
    QPointer<PressMotion> guard(this);
    if (! v)
        snapTo(0);
    else if (m_pressed)
        setTargetValue(1);
    if (guard) Q_EMIT activeChanged();
}
void PressMotion::releaseIfReady() {
    if (m_releasing && (value() > .75 || elapsed() >= 1000 || ! isRunning())) {
        m_releasing = false;
        setTargetValue(0);
    }
}
} // namespace qml_material
