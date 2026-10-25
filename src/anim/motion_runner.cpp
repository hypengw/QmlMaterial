#include "qml_material/anim/motion_runner.hpp"
#include <QPointer>
#include <algorithm>
#include <cmath>
#include <limits>

namespace qml_material
{
class MotionRunner::Clock : public QAbstractAnimation {
public:
    explicit Clock(MotionRunner* owner): QAbstractAnimation(owner), owner(owner) {}
    int duration() const override { return -1; }
    int previous = 0;

protected:
    void updateCurrentTime(int t) override {
        const auto delta = t - previous;
        previous         = t;
        if (delta > 0) owner->tick(delta);
    }

private:
    MotionRunner* owner;
};
MotionRunner::MotionRunner(int dimensions, QObject* parent)
    : QObject(parent),
      m_clock(new Clock(this)),
      m_sample { anim::Vector(dimensions, 0), anim::Vector(dimensions, 0) },
      m_target(dimensions, 0),
      m_lower(dimensions, -std::numeric_limits<qreal>::infinity()),
      m_upper(dimensions, std::numeric_limits<qreal>::infinity()) {}
MotionRunner::~MotionRunner() { m_clock->stop(); }
QString MotionRunner::nextId() { return QString::number(++m_sequence); }
bool    MotionRunner::validVector(const anim::Vector& value) const {
    return value.size() == m_sample.value.size() && anim::finite(value);
}
bool MotionRunner::clamp(anim::Vector& value) const {
    bool changed = false;
    for (int i = 0; i < value.size(); ++i) {
        const auto v = std::clamp(value[i], m_lower[i], m_upper[i]);
        changed |= v != value[i];
        value[i] = v;
    }
    return changed;
}
void MotionRunner::notify() { Q_EMIT changed(); }
void MotionRunner::terminate(Reason reason) {
    if (m_state != Idle && m_state != Interacting) {
        const auto   id = m_id;
        QVariantList value, velocity;
        for (auto v : m_sample.value) value.append(v);
        for (auto v : m_sample.velocity) velocity.append(v);
        QMetaObject::invokeMethod(
            this,
            [this, id, reason, value, velocity] {
                Q_EMIT ended(id, reason, value, velocity);
            },
            Qt::QueuedConnection);
    }
    m_clock->stop();
    m_state = Idle;
    m_id.clear();
    m_sample.velocity.fill(0);
}
QString MotionRunner::start(anim::PreparedMotion motion) {
    terminate(Interrupted);
    m_motion      = std::move(motion);
    m_target      = m_motion.target();
    m_elapsed     = 0;
    m_sample      = m_motion.sample(0);
    m_state       = Running;
    const auto id = m_id = nextId();
    if (m_motion.duration() == 0 || m_scale == 0) {
        m_sample           = m_motion.sample(m_motion.duration());
        const bool bounded = clamp(m_sample.value);
        terminate(bounded ? BoundReached : Finished);
    } else if (! m_manual) {
        m_clock->previous = 0;
        m_clock->start();
    }
    notify();
    return id;
}
QString MotionRunner::animateTo(const anim::Vector& target, AnimationSpec spec,
                                std::optional<anim::Vector> velocity,
                                const anim::Vector& thresholds, anim::SpringCompletion completion) {
    if (m_state == Interacting || ! validVector(target)) return {};
    const auto v = velocity.value_or(m_sample.velocity);
    if (! validVector(v)) return {};
    auto motion =
        anim::PreparedMotion::prepare(m_sample.value, target, v, spec, thresholds, completion);
    return motion ? start(std::move(*motion)) : QString();
}
QString MotionRunner::animateDecay(const anim::Vector& velocity, DecaySpec spec) {
    if (m_state == Interacting || ! validVector(velocity)) return {};
    auto motion = anim::PreparedMotion::decay(m_sample.value, velocity, spec);
    return motion ? start(std::move(*motion)) : QString();
}
bool MotionRunner::snapTo(const anim::Vector& value) {
    if (m_state == Interacting || ! validVector(value)) return false;
    terminate(Interrupted);
    m_sample.value = value;
    clamp(m_sample.value);
    m_target  = m_sample.value;
    m_elapsed = 0;
    notify();
    return true;
}
void MotionRunner::tick(qreal dt) {
    if (m_state != Running || ! std::isfinite(dt) || dt < 0) return;
    m_elapsed = m_scale == 0 ? m_motion.duration()
                             : std::min(qreal(m_motion.duration()), m_elapsed + dt / m_scale);
    m_sample  = m_motion.sample(m_elapsed);
    if (clamp(m_sample.value))
        terminate(BoundReached);
    else if (m_elapsed >= m_motion.duration())
        terminate(Finished);
    notify();
}
void MotionRunner::setPlayTime(qreal ms) {
    if (! m_manual || ! std::isfinite(ms) || ms < 0 || m_state != Running) return;
    m_elapsed = 0;
    tick(ms * m_scale);
}
void MotionRunner::advance(qreal ms) {
    if (m_manual) tick(ms);
}
void MotionRunner::setManualClock(bool v) {
    if (v == m_manual) return;
    m_manual = v;
    m_clock->stop();
    if (! v && m_state == Running) {
        m_clock->previous = 0;
        m_clock->start();
    }
}
void MotionRunner::setDurationScale(qreal value) {
    if (! std::isfinite(value) || value < 0 || value == m_scale) return;
    m_scale = value;
    if (value == 0 && (m_state == Running || m_state == Paused)) complete();
}
bool MotionRunner::setBounds(anim::Vector lower, anim::Vector upper) {
    if (lower.size() != m_lower.size() || upper.size() != m_upper.size()) return false;
    for (int i = 0; i < lower.size(); ++i)
        if (std::isnan(lower[i]) || std::isnan(upper[i]) || lower[i] > upper[i]) return false;
    m_lower = std::move(lower);
    m_upper = std::move(upper);
    if (clamp(m_sample.value)) {
        if (m_state != Interacting) terminate(BoundReached);
        notify();
    }
    return true;
}
void MotionRunner::pause() {
    if (m_state != Running) return;
    m_clock->stop();
    m_state = Paused;
    notify();
}
void MotionRunner::resume() {
    if (m_state != Paused) return;
    m_state = Running;
    if (! m_manual) {
        m_clock->previous = 0;
        m_clock->start();
    }
    notify();
}
void MotionRunner::stop(const QString& expected) {
    if (m_state == Idle || m_state == Interacting || (! expected.isEmpty() && expected != m_id))
        return;
    terminate(Cancelled);
    notify();
}
void MotionRunner::complete(const QString& expected) {
    if (m_state == Idle || m_state == Interacting || (! expected.isEmpty() && expected != m_id))
        return;
    m_elapsed          = m_motion.duration();
    m_sample           = m_motion.sample(m_elapsed);
    const auto bounded = clamp(m_sample.value);
    terminate(bounded ? BoundReached : Completed);
    notify();
}
QString MotionRunner::beginInteraction() {
    if (m_state == Interacting) return {};
    terminate(Interrupted);
    m_state       = Interacting;
    const auto id = m_id = nextId();
    notify();
    return id;
}
bool MotionRunner::updateInteraction(const QString& id, const anim::Vector& value) {
    if (m_state != Interacting || id != m_id || ! validVector(value)) return false;
    m_sample.value = value;
    clamp(m_sample.value);
    notify();
    return true;
}
bool MotionRunner::cancelInteraction(const QString& id) {
    if (m_state != Interacting || id != m_id) return false;
    terminate(Cancelled);
    notify();
    return true;
}
QString MotionRunner::settleInteraction(const QString& id, const anim::Vector& target,
                                        const anim::Vector& velocity, AnimationSpec spec) {
    if (m_state != Interacting || id != m_id || ! validVector(target) || ! validVector(velocity))
        return {};
    auto motion = anim::PreparedMotion::prepare(m_sample.value, target, velocity, spec);
    return motion ? start(std::move(*motion)) : QString();
}
} // namespace qml_material
