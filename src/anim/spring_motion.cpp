#include "qml_material/anim/spring_motion.hpp"
#include "qml_material/math/color.hpp"
#include <QPointer>
#include <QSignalBlocker>
#include <QtQml/private/qqmldata_p.h>
#include "qml_material/style/theme.hpp"
#include "qml_material/style/common_state.hpp"
#include "qml_material/util/qt.hpp"
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
void SpringMotionBase::setTargets(const QList<qreal>& targets, bool snap, bool preserveVelocity,
                                  std::optional<anim::Vector> from,
                                  std::optional<anim::Vector> initialVelocity) {
    if (targets.size() != m_targets.size() || ! anim::finite(targets) ||
        (targets == m_targets && ! snap && ! from))
        return;
    const auto revision = ++m_revision;
    const bool changed  = targets != m_targets;
    m_targets           = targets;
    const QPointer<SpringMotionBase> guard(this);
    if (from) {
        m_notifyFinish = false;
        // Normalizing endpoints is one retarget, not an observable stop/start pair.
        const QSignalBlocker blocker(&m_runner);
        m_runner.snapTo(*from);
        if (! guard || revision != m_revision) return;
    }
    m_notifyFinish = ! snap && m_enabled && m_complete;
    if (! m_notifyFinish)
        m_runner.snapTo(targets);
    else {
        auto         spec = effectiveSpec();
        anim::Vector thresholds;
        for (int i = 0; i < targets.size(); ++i) thresholds.append(displacementThreshold(i));
        m_runner.animateTo(targets,
                           spec,
                           initialVelocity.value_or(preserveVelocity
                                                        ? m_runner.sample().velocity
                                                        : anim::Vector(targets.size(), 0)),
                           thresholds,
                           springCompletion());
    }
    if (! guard || revision != m_revision) return;
    m_notifyFinish = true;
    if (changed) emit targetChanged();
}
void SpringMotionBase::restart() {
    auto         spec = effectiveSpec();
    anim::Vector thresholds;
    for (int i = 0; i < m_targets.size(); ++i) thresholds.append(displacementThreshold(i));
    m_runner.animateTo(m_targets, spec, {}, thresholds, springCompletion());
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
CornerSpringMotion::CornerSpringMotion(QObject* parent): SpringMotionBase(1, parent) {
    useEffects();
    connect(this, &SpringMotionBase::valueChanged, this, &CornerSpringMotion::notifyCorners);
}
CommonState* CornerSpringMotion::source() const { return m_source; }
void         CornerSpringMotion::setSource(CommonState* source) {
    if (source == m_source) return;
    utils::disconnectAll(m_connections);
    m_source = source;
    if (source) {
        m_connections.append(connect(
            source, &CommonState::cornerShapeChanged, this, &CornerSpringMotion::updateShape));
        m_connections.append(connect(source, &CommonState::targetChanged, this, [this] {
            updateShape(true);
        }));
        m_connections.append(connect(source, &QObject::destroyed, this, [this] {
            m_source = nullptr;
            if (QQmlData::wasDeleted(this)) return;
            const QPointer<CornerSpringMotion> guard(this);
            setShape({}, true);
            if (guard) Q_EMIT sourceChanged();
        }));
    }
    const QPointer<CornerSpringMotion> guard(this);
    updateShape(true);
    if (guard) Q_EMIT sourceChanged();
}
void CornerSpringMotion::updateShape(bool snap) {
    setShape(m_source ? m_source->cornerShape() : CornerShape {}, snap);
}
CornersGroup CornerSpringMotion::targetCorners() const {
    auto shape   = m_to;
    shape.extent = m_extent;
    return shape.resolved();
}
void CornerSpringMotion::setTargetCorners(const CornersGroup& v) { setShape({ v }); }
void CornerSpringMotion::setShape(CornerShape shape, bool snap) {
    for (int i = 0; i < 4; ++i)
        if (! std::isfinite(shape.absolute[i]) || ! std::isfinite(shape.relative[i])) return;
    if (! std::isfinite(shape.extent) || shape.extent < 0) return;
    const auto oldExtent = m_extent;
    if (oldExtent != shape.extent && ! shape.sameShape(m_to) && shape.resolved() == corners())
        snap = true;
    const QPointer<CornerSpringMotion> guard(this);
    if (! snap && shape.sameShape(m_to)) {
        m_extent = shape.extent;
        if (oldExtent != m_extent) {
            Q_EMIT valueChanged();
            if (guard) Q_EMIT targetChanged();
        }
        return;
    }
    const auto progress  = value(0);
    const auto speed     = velocity(0);
    const bool reversing = shape.sameShape(m_from);
    if (snap) {
        m_from = m_to = shape;
    } else if (reversing) {
        m_from = m_to;
        m_to   = shape;
    } else {
        m_from = m_from.interpolated(m_to, progress);
        m_to   = shape;
    }
    m_extent = shape.extent;
    setTargets({ 1 },
               snap,
               false,
               anim::Vector { snap        ? 1
                              : reversing ? 1 - progress
                                          : 0 },
               anim::Vector { ! snap && reversing ? -speed : 0 });
    if (! guard) return;
    Q_EMIT targetChanged();
    if (guard) Q_EMIT valueChanged();
}
CornersGroup CornerSpringMotion::corners() const {
    auto shape   = m_from.interpolated(m_to, value(0));
    shape.extent = m_extent;
    auto result  = shape.resolved();
    for (int i = 0; i < 4; ++i) result[i] = std::max(qreal(0), result[i]);
    return result;
}
void CornerSpringMotion::notifyCorners() {
    if (QQmlData::wasDeleted(this)) return;
    const auto next    = corners();
    bool       changed = false;
    for (int i = 0; i < 4; ++i) changed |= std::abs(next[i] - m_notifiedCorners[i]) > 1e-9;
    if (! changed) return;
    m_notifiedCorners = next;
    Q_EMIT cornersChanged();
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
