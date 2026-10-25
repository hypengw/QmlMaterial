#include "qml_material/anim/spring_motion.hpp"
#include "qml_material/math/color.hpp"
#include <QPointer>
#include "qml_material/style/theme.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace qml_material
{
SpringMotionBase::SpringMotionBase(int dimensions, QObject* parent)
    : QObject(parent),
      m_runner(dimensions, this),
      m_targets(dimensions, 0),
      m_notified { m_targets, m_targets } {
    connect(&m_runner, &MotionRunner::changed, this, &SpringMotionBase::synchronize);
}
SpringMotionBase::~SpringMotionBase() = default;
void SpringMotionBase::classBegin() { m_complete = false; }
void SpringMotionBase::componentComplete() {
    m_complete = true;
    if (qmlEngine(this)) {
        m_settings = qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(this, true))->motion();
        connect(m_settings, &MotionSettings::changed, this, &SpringMotionBase::refreshPolicy);
        refreshPolicy();
    }
}
AnimationSpec SpringMotionBase::effectiveSpec() const {
    return ! m_explicitSpec && m_settings
               ? (m_effects ? m_settings->effects() : m_settings->spatial())
               : Motion::spring(m_spec.stiffness, m_spec.dampingRatio);
}
token::SpringSpec SpringMotionBase::spec() const {
    const auto s = effectiveSpec();
    return { s.stiffness, s.dampingRatio };
}
void SpringMotionBase::refreshPolicy() {
    const QPointer<SpringMotionBase> guard(this);
    m_runner.setDurationScale(
        m_settings ? (m_settings->reducedMotion() ? 0 : m_settings->durationScale()) : 1);
    if (guard) emit specChanged();
}
qreal SpringMotionBase::target(int i) const { return m_targets[i]; }
qreal SpringMotionBase::value(int i) const { return m_runner.sample().value[i]; }
qreal SpringMotionBase::velocity(int i) const { return m_runner.sample().velocity[i]; }
int   SpringMotionBase::elapsed() const { return int(m_runner.elapsed()); }
qreal SpringMotionBase::displacementThreshold(int) const { return m_threshold; }
void  SpringMotionBase::synchronize() {
    const auto revision                       = m_revision;
    const bool wasRunning                     = m_notifiedRunning;
    m_notifiedRunning                         = m_runner.active();
    const bool                       finished = wasRunning && ! m_notifiedRunning && m_notifyFinish;
    const QPointer<SpringMotionBase> guard(this);
    const bool valueChanged = m_notified.value != m_runner.sample().value ||
                              m_notified.velocity != m_runner.sample().velocity;
    m_notified              = m_runner.sample();
    if (valueChanged) emit this->valueChanged();
    if (! guard || revision != m_revision) return;
    if (wasRunning != m_notifiedRunning) emit runningChanged();
    if (guard && revision == m_revision && finished) emit this->finished();
}
void SpringMotionBase::setTargets(const QList<qreal>& targets, bool snap, bool preserveVelocity) {
    if (targets.size() != m_targets.size() || ! anim::finite(targets) ||
        (targets == m_targets && ! snap))
        return;
    const auto revision = ++m_revision;
    const bool changed  = targets != m_targets;
    m_targets           = targets;
    const QPointer<SpringMotionBase> guard(this);
    m_notifyFinish = ! snap && m_enabled && m_complete;
    if (! m_notifyFinish)
        m_runner.snapTo(targets);
    else {
        auto         spec = effectiveSpec();
        anim::Vector thresholds;
        for (int i = 0; i < targets.size(); ++i) thresholds.append(displacementThreshold(i));
        m_runner.animateTo(targets,
                           spec,
                           preserveVelocity ? m_runner.sample().velocity
                                            : anim::Vector(targets.size(), 0),
                           thresholds);
    }
    if (! guard || revision != m_revision) return;
    m_notifyFinish = true;
    if (changed) emit targetChanged();
}
void SpringMotionBase::restart() {
    auto         spec = effectiveSpec();
    anim::Vector thresholds;
    for (int i = 0; i < m_targets.size(); ++i) thresholds.append(displacementThreshold(i));
    m_runner.animateTo(m_targets, spec, {}, thresholds);
}
void SpringMotionBase::setSpec(token::SpringSpec spec) {
    if (! spec.valid()) return;
    m_explicitSpec = true;
    if (spec == m_spec) return;
    m_spec                                    = spec;
    const auto                       revision = ++m_revision;
    const QPointer<SpringMotionBase> guard(this);
    if (m_runner.active()) restart();
    if (guard && revision == m_revision) emit specChanged();
}
void SpringMotionBase::setVisibilityThreshold(qreal v) {
    if (! std::isfinite(v) || v <= 0 || v == m_threshold) return;
    m_threshold                               = v;
    const auto                       revision = ++m_revision;
    const QPointer<SpringMotionBase> guard(this);
    if (m_runner.active()) restart();
    if (guard && revision == m_revision) emit visibilityThresholdChanged();
}
void SpringMotionBase::setEnabled(bool v) {
    if (v == m_enabled) return;
    m_enabled                                 = v;
    const auto                       revision = ++m_revision;
    const QPointer<SpringMotionBase> guard(this);
    if (! v) {
        m_notifyFinish = false;
        m_runner.snapTo(m_targets);
    }
    if (! guard || revision != m_revision) return;
    m_notifyFinish = true;
    emit enabledChanged();
}
void SpringMotionBase::complete() {
    if (! m_runner.active()) {
        if (m_runner.sample().value != m_targets) {
            ++m_revision;
            m_runner.snapTo(m_targets);
        }
        return;
    }
    ++m_revision;
    m_runner.complete();
}
void SpringMotionBase::stop() {
    if (! m_runner.active()) return;
    const auto revision = ++m_revision;
    m_notifyFinish      = false;
    const QPointer<SpringMotionBase> guard(this);
    m_runner.stop();
    if (guard && revision == m_revision) m_notifyFinish = true;
}
SpringMotion::SpringMotion(QObject* parent): SpringMotionBase(1, parent) {}
ColorSpringMotion::ColorSpringMotion(QObject* parent): SpringMotionBase(4, parent) { useEffects(); }
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
    useEffects();
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
