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
        Target
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
        const double distance = m_start - m_target;
        const double b        = m_velocity + m_rate * distance;
        const double decay    = std::exp(-m_rate * elapsed);
        const double offset   = (distance + b * elapsed) * decay;
        const double velocity = (b - m_rate * (distance + b * elapsed)) * decay;
        if (std::abs(offset) < 0.01 && std::abs(velocity) < 0.1) return { m_target, 0, true };
        return { m_target + offset, velocity, false };
    }

private:
    unsigned long long m_revision { 0 };
    Mode               m_mode { Mode::Idle };
    double             m_start { 0 };
    double             m_velocity { 0 };
    double             m_target { 0 };
    double             m_time { 0 };
    double             m_rate { 1 };
};

} // namespace qml_material
