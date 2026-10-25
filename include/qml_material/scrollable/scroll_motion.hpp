#pragma once

#include <algorithm>
#include <cmath>

namespace qml_material
{

class ScrollMotion {
public:
    enum class Mode
    {
        Idle,
        Fling,
        Target,
        Wheel
    };
    struct Sample {
        double position;
        double velocity;
        bool   finished;
    };

    Mode               mode() const { return m_mode; }
    bool               active() const { return m_mode != Mode::Idle; }
    double             target() const { return m_target; }
    unsigned long long revision() const { return m_revision; }

    void stop() {
        ++m_revision;
        m_mode = Mode::Idle;
    }

    void fling(double position, double velocity, double deceleration, double now) {
        ++m_revision;
        m_mode     = Mode::Fling;
        m_start    = position;
        m_velocity = velocity;
        m_rate     = std::max(1.0, deceleration);
        m_time     = now;
        m_target   = position + velocity * std::abs(velocity) / (2 * m_rate);
    }

    void smooth(double position, double velocity, double target, double now) {
        ++m_revision;
        m_mode     = Mode::Target;
        m_start    = position;
        m_velocity = velocity;
        m_target   = target;
        m_time     = now;
        m_rate     = 18;
    }

    void wheel(double position, double target, double now) {
        const auto previous = m_mode == Mode::Wheel ? sample(now) : Sample { position, 0, true };
        if (m_mode == Mode::Wheel && target == m_target) return;
        ++m_revision;
        m_mode                = Mode::Wheel;
        m_start               = previous.position;
        m_velocity            = previous.velocity;
        m_target              = target;
        m_time                = now;
        const double distance = target - m_start;
        // Chromium's inverse-distance duration; bound the initial slope to keep the curve
        // monotonic.
        m_duration = std::clamp((840.0 - std::abs(distance)) / 3600.0, 0.1, 0.2);
        if (distance * m_velocity > 0)
            m_duration = std::min(m_duration, distance / (0.42 * m_velocity));
        if (std::abs(distance) < 0.01) m_duration = 0;
        m_slope =
            m_duration > 0 ? std::clamp(m_velocity * m_duration / distance, -1000.0, 1000.0) : 0;
    }

    void translate(double delta) {
        ++m_revision;
        m_start += delta;
        m_target += delta;
    }

    Sample sample(double now) const {
        const double elapsed = std::max(0.0, now - m_time);
        if (m_mode == Mode::Idle) return { m_start, 0, true };
        if (m_mode == Mode::Fling) {
            const double duration = std::abs(m_velocity) / m_rate;
            if (elapsed >= duration) return { m_target, 0, true };
            const double acceleration = -std::copysign(m_rate, m_velocity);
            return { m_start + m_velocity * elapsed + acceleration * elapsed * elapsed / 2,
                     m_velocity + acceleration * elapsed,
                     false };
        }
        if (m_mode == Mode::Wheel) {
            if (elapsed >= m_duration) return { m_target, 0, true };
            if (elapsed == 0) return { m_start, m_velocity, false };
            const double progress = elapsed / m_duration;
            // Solve the monotonic time axis of cubic-bezier(0.42, slope * 0.42, 0.58, 1).
            double low = 0, high = 1;
            for (int i = 0; i < 40; ++i) {
                const double t = (low + high) / 2;
                if (bezier(t, 0.42, 0.58) < progress)
                    low = t;
                else
                    high = t;
            }
            const double t        = (low + high) / 2;
            const double distance = m_target - m_start;
            return { m_start + distance * bezier(t, m_slope * 0.42, 1),
                     distance / m_duration * derivative(t, m_slope * 0.42, 1) /
                         derivative(t, 0.42, 0.58),
                     false };
        }
        const double distance = m_start - m_target;
        const double b        = m_velocity + m_rate * distance;
        const double decay    = std::exp(-m_rate * elapsed);
        const double offset   = (distance + b * elapsed) * decay;
        const double velocity = (b - m_rate * (distance + b * elapsed)) * decay;
        if (std::abs(offset) < 0.01 && std::abs(velocity) < 0.1) return { m_target, 0, true };
        return { m_target + offset, velocity, false };
    }

private:
    static double bezier(double t, double a, double b) {
        const double u = 1 - t;
        return 3 * u * u * t * a + 3 * u * t * t * b + t * t * t;
    }
    static double derivative(double t, double a, double b) {
        const double u = 1 - t;
        return 3 * u * u * a + 6 * u * t * (b - a) + 3 * t * t * (1 - b);
    }
    unsigned long long m_revision { 0 };
    Mode               m_mode { Mode::Idle };
    double             m_start { 0 };
    double             m_velocity { 0 };
    double             m_target { 0 };
    double             m_time { 0 };
    double             m_rate { 1 };
    double             m_duration { 0 };
    double             m_slope { 0 };
};

} // namespace qml_material
