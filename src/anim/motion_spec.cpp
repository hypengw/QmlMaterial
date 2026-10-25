#include "qml_material/anim/motion_spec.hpp"
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

bool AnimationSpec::valid() const {
    return kind >= Tween && kind <= Snap && duration >= 0 && delay >= 0 && duration <= 1000000000 &&
           delay <= 1000000000 && token::SpringSpec { stiffness, dampingRatio }.valid() &&
           std::isfinite(threshold) && threshold > 0 && std::isfinite(velocityThreshold) &&
           velocityThreshold > 0;
}
bool DecaySpec::valid() const {
    return std::isfinite(friction) && friction > 0 && std::isfinite(velocityThreshold) &&
           velocityThreshold > 0;
}
AnimationSpec Motion::tween(int duration, QEasingCurve easing, int delay) {
    AnimationSpec s;
    s.kind     = AnimationSpec::Tween;
    s.duration = duration;
    s.easing   = easing;
    s.delay    = delay;
    return s;
}
AnimationSpec Motion::spring(qreal stiffness, qreal dampingRatio) {
    AnimationSpec s;
    s.stiffness    = stiffness;
    s.dampingRatio = dampingRatio;
    return s;
}
AnimationSpec Motion::snap() {
    AnimationSpec s;
    s.kind = AnimationSpec::Snap;
    return s;
}
DecaySpec Motion::decay(qreal friction, qreal threshold) { return { friction, threshold }; }
namespace anim
{
bool finite(const Vector& v) {
    return ! v.isEmpty() && std::all_of(v.begin(), v.end(), [](qreal x) {
        return std::isfinite(x);
    });
}
std::optional<PreparedMotion> PreparedMotion::prepare(const Vector& from, const Vector& to,
                                                      const Vector& velocity, AnimationSpec spec,
                                                      const Vector&    thresholds,
                                                      SpringCompletion completion) {
    if (! finite(from) || ! finite(to) || ! finite(velocity) || from.size() != to.size() ||
        from.size() != velocity.size() || ! spec.valid() ||
        (! thresholds.isEmpty() && (thresholds.size() != from.size() || ! finite(thresholds))))
        return {};
    PreparedMotion m;
    m.m_from     = from;
    m.m_to       = to;
    m.m_velocity = velocity;
    m.m_spec     = spec;
    for (int i = 0; i < from.size(); ++i) {
        if (! thresholds.isEmpty() && thresholds[i] <= 0) return {};
        int duration = 0;
        if (spec.kind == AnimationSpec::Tween) duration = from[i] == to[i] ? 0 : spec.duration;
        if (spec.kind == AnimationSpec::Spring) {
            const token::SpringSpec spring { spec.stiffness, spec.dampingRatio };
            const auto              acceleration =
                -spec.stiffness * (from[i] - to[i]) -
                2 * spec.dampingRatio * std::sqrt(spec.stiffness) * velocity[i];
            if (! std::isfinite(acceleration)) return {};
            duration = springDuration(spring,
                                      from[i],
                                      to[i],
                                      velocity[i],
                                      thresholds.isEmpty() ? spec.threshold : thresholds[i]);
            if (completion == SpringCompletion::DisplacementAndVelocity)
                duration =
                    std::max(duration,
                             springDuration(
                                 spring,
                                 velocity[i],
                                 0,
                                 acceleration,
                                 spec.velocityThreshold *
                                     (thresholds.isEmpty() ? 1 : thresholds[i] / spec.threshold)));
        }
        duration = std::min(duration, 1000000000);
        m.m_durations.append(duration);
        m.m_duration = std::max(m.m_duration, duration);
    }
    if (m.m_duration) m.m_duration += spec.delay;
    return m;
}
std::optional<PreparedMotion> PreparedMotion::decay(const Vector& from, const Vector& velocity,
                                                    DecaySpec spec) {
    if (! finite(from) || ! finite(velocity) || from.size() != velocity.size() || ! spec.valid())
        return {};
    PreparedMotion m;
    m.m_from     = from;
    m.m_velocity = velocity;
    m.m_decay    = spec;
    for (int i = 0; i < from.size(); ++i) {
        const auto target = from[i] + velocity[i] / spec.friction;
        if (! std::isfinite(target)) return {};
        m.m_to.append(target);
        const auto seconds = std::max(
            qreal(0), std::log(std::abs(velocity[i]) / spec.velocityThreshold) / spec.friction);
        const int duration = int(std::min(qreal(1000000000), std::ceil(seconds * 1000)));
        m.m_durations.append(duration);
        m.m_duration = std::max(m.m_duration, duration);
    }
    return m;
}
Sample PreparedMotion::sample(qreal milliseconds) const {
    Sample     s { m_from, Vector(m_from.size(), 0) };
    const auto time = std::max(qreal(0), milliseconds - m_spec.delay);
    for (int i = 0; i < m_from.size(); ++i) {
        const auto duration = m_durations[i];
        if (milliseconds >= m_duration || (milliseconds >= m_spec.delay && time >= duration)) {
            s.value[i] = m_to[i];
            continue;
        }
        if (milliseconds < m_spec.delay) continue;
        if (m_decay) {
            s.velocity[i] = m_velocity[i] * std::exp(-m_decay->friction * time / 1000);
            s.value[i]    = m_from[i] + (m_velocity[i] - s.velocity[i]) / m_decay->friction;
        } else if (m_spec.kind == AnimationSpec::Spring) {
            const auto sample = sampleSpring({ m_spec.stiffness, m_spec.dampingRatio },
                                             m_from[i],
                                             m_to[i],
                                             m_velocity[i],
                                             time / 1000);
            s.value[i]        = sample.value;
            s.velocity[i]     = sample.velocity;
        } else if (m_spec.kind == AnimationSpec::Tween && duration > 0) {
            const auto p  = std::clamp(time / duration, qreal(0), qreal(1));
            const auto lo = std::max(qreal(0), p - 1e-5), hi = std::min(qreal(1), p + 1e-5);
            const auto delta = m_to[i] - m_from[i];
            s.value[i]       = m_from[i] + delta * m_spec.easing.valueForProgress(p);
            s.velocity[i] =
                delta * (m_spec.easing.valueForProgress(hi) - m_spec.easing.valueForProgress(lo)) /
                (hi - lo) * 1000 / duration;
        }
    }
    return s;
}
} // namespace anim
} // namespace qml_material
