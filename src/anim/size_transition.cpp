#include "qml_material/anim/size_transition.hpp"
#include "qml_material/style/theme.hpp"
#include "qml_material/token/token.hpp"
#include <cmath>
#include <limits>
namespace qml_material
{
SizeTransition::SizeTransition(QObject* p): QObject(p) {
    connect(&m_runner, &MotionRunner::changed, this, &SizeTransition::synchronize);
}
AnimationSpec SizeTransition::spec() const {
    return m_spec.value_or(m_settings ? m_settings->container()
                                      : Motion::tween(450, token::Easing {}.emphasized()));
}
QSizeF SizeTransition::bounded(QSizeF v) const {
    return { std::max(qreal(0),
                      m_maximum.width() < 0 ? v.width() : std::min(v.width(), m_maximum.width())),
             std::max(qreal(0),
                      m_maximum.height() < 0 ? v.height()
                                             : std::min(v.height(), m_maximum.height())) };
}
void SizeTransition::synchronize() {
    if (m_configuring) return;
    const auto   revision = m_revision;
    const auto&  v        = m_runner.sample().value;
    const QSizeF size(v[0], v[1]);
    const bool   resized = size != m_size, activity = m_active != m_runner.active();
    const bool   running = m_running != (m_runner.state() == MotionRunner::Running);
    m_size               = size;
    m_active             = m_runner.active();
    m_running            = m_runner.state() == MotionRunner::Running;
    const QPointer<SizeTransition> guard(this);
    if (resized) emit sizeChanged();
    if (! guard || revision != m_revision) return;
    if (running) emit runningChanged();
    if (guard && revision == m_revision && activity) emit activeChanged();
}
void SizeTransition::setTargetSize(QSizeF v) {
    if (! std::isfinite(v.width()) || ! std::isfinite(v.height()) || v.width() < 0 ||
        v.height() < 0 || (m_hasTarget && v == m_target))
        return;
    m_hasTarget = true;
    m_target    = v;
    const QPointer<SizeTransition> guard(this);
    emit                           targetSizeChanged();
    if (guard) retarget();
}
void SizeTransition::setMaximumSize(QSizeF v) {
    if (! std::isfinite(v.width()) || ! std::isfinite(v.height()) || v.width() < -1 ||
        v.height() < -1 || v == m_maximum)
        return;
    m_maximum           = v;
    const auto infinity = std::numeric_limits<qreal>::infinity();
    m_configuring       = true;
    m_runner.setBounds(
        { 0, 0 }, { v.width() < 0 ? infinity : v.width(), v.height() < 0 ? infinity : v.height() });
    m_configuring = false;
    const QPointer<SizeTransition> guard(this);
    emit                           maximumSizeChanged();
    if (guard) retarget();
}
void SizeTransition::setEnabled(bool v) {
    if (v == m_enabled) return;
    m_enabled = v;
    const QPointer<SizeTransition> guard(this);
    emit                           enabledChanged();
    if (guard) retarget();
}
void SizeTransition::setSpec(AnimationSpec v) {
    if (! v.valid() || m_spec == v) return;
    m_spec = v;
    const QPointer<SizeTransition> guard(this);
    emit                           specChanged();
    if (guard) retarget(true);
}
void SizeTransition::resetSpec() {
    m_spec.reset();
    const QPointer<SizeTransition> guard(this);
    emit                           specChanged();
    if (guard) retarget(true);
}
void SizeTransition::retarget(bool force) {
    ++m_revision;
    if (! m_seek.isEmpty()) {
        synchronize();
        return;
    }
    const auto         target = bounded(m_target);
    const anim::Vector v { target.width(), target.height() };
    m_configuring = true;
    if (! m_complete || ! m_initialized || ! m_enabled || m_runner.sample().value == v) {
        m_runner.snapTo(v);
        m_initialized = m_complete && m_hasTarget;
    } else if (force || ! m_runner.active() || m_runner.target() != v)
        m_runner.animateTo(v, spec());
    m_configuring = false;
    synchronize();
}
void SizeTransition::complete() {
    ++m_revision;
    if (! m_seek.isEmpty()) return;
    const auto v  = bounded(m_target);
    m_configuring = true;
    m_runner.snapTo({ v.width(), v.height() });
    m_configuring = false;
    synchronize();
}
void SizeTransition::componentComplete() {
    m_complete = true;
    const QPointer<SizeTransition> guard(this);
    if (! m_settings && qmlEngine(this))
        setSettings(qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(this, true))->motion());
    if (! guard) return;
    m_initialized = true;
    complete();
}
QString SizeTransition::beginSeek(QSizeF destination) {
    if (! m_seek.isEmpty()) return m_seek;
    const auto to     = bounded(destination);
    auto       motion = anim::PreparedMotion::prepare(
        m_runner.sample().value, { to.width(), to.height() }, { 0, 0 }, spec());
    if (! motion) return {};
    m_seekMotion  = *motion;
    m_seekOrigin  = m_size;
    m_configuring = true;
    m_seek        = m_runner.beginInteraction();
    const auto id = m_seek;
    m_configuring = false;
    synchronize();
    return id;
}
bool SizeTransition::seek(QString id, qreal progress) {
    if (id.isEmpty() || id != m_seek || ! std::isfinite(progress)) return false;
    return m_runner.updateInteraction(
        id,
        m_seekMotion.sample(std::clamp(progress, qreal(0), qreal(1)) * m_seekMotion.duration())
            .value);
}
void SizeTransition::endSeek(QString id, bool commit) {
    if (id.isEmpty() || id != m_seek) return;
    ++m_revision;
    m_configuring = true;
    m_runner.cancelInteraction(id);
    m_seek.clear();
    const auto target = bounded(commit ? m_target : m_seekOrigin);
    if (m_enabled)
        m_runner.animateTo({ target.width(), target.height() }, spec());
    else
        m_runner.snapTo({ target.width(), target.height() });
    m_configuring = false;
    synchronize();
}
void SizeTransition::setSettings(MotionSettings* settings) {
    if (settings == m_settings) return;
    disconnect(m_policy);
    disconnect(m_policyDestroyed);
    m_settings = settings;
    if (settings) {
        m_policy =
            connect(settings, &MotionSettings::changed, this, &SizeTransition::refreshPolicy);
        m_policyDestroyed = connect(settings, &QObject::destroyed, this, [this] {
            m_settings = nullptr;
            refreshPolicy();
        });
    }
    const QPointer<SizeTransition> guard(this);
    refreshPolicy();
    if (guard) emit settingsChanged();
}
void SizeTransition::refreshPolicy() {
    const QPointer<SizeTransition> guard(this);
    m_runner.setDurationScale(
        m_settings ? (m_settings->reducedMotion() ? 0 : m_settings->durationScale()) : 1);
    if (guard) emit specChanged();
}
} // namespace qml_material
