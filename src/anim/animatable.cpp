#include "qml_material/anim/animatable.hpp"
#include "qml_material/math/color.hpp"
#include "qml_material/style/theme.hpp"
#include <QQmlEngine>
namespace qml_material
{
namespace anim
{
std::optional<Vector> encode(const QVariant& v, ValueKind kind) {
    Vector result;
    if (kind == ValueKind::Real) {
        bool       ok = false;
        const auto x  = v.toDouble(&ok);
        if (! ok) return {};
        result = { x };
    } else if (kind == ValueKind::Point) {
        if (! v.canConvert<QPointF>()) return {};
        const auto p = v.toPointF();
        result       = { p.x(), p.y() };
    } else if (kind == ValueKind::Size) {
        if (! v.canConvert<QSizeF>()) return {};
        const auto p = v.toSizeF();
        result       = { p.width(), p.height() };
    } else {
        if (! v.canConvert<QColor>()) return {};
        const auto c = v.value<QColor>();
        if (! c.isValid()) return {};
        const auto lab = math::toOklab(c);
        result         = { c.alphaF(), lab[0], lab[1], lab[2] };
    }
    return finite(result) ? std::optional(result) : std::nullopt;
}
QVariant decode(const Vector& v, ValueKind kind) {
    switch (kind) {
    case ValueKind::Real: return v[0];
    case ValueKind::Point: return QPointF(v[0], v[1]);
    case ValueKind::Size: return QSizeF(v[0], v[1]);
    case ValueKind::Color:
        return math::fromOklab({ v[1], v[2], v[3] }, std::clamp(v[0], qreal(0), qreal(1)));
    }
    return {};
}
QVariant velocityValue(const Vector& v, ValueKind kind) {
    return kind == ValueKind::Color ? QVariant::fromValue(QVector4D(v[0], v[1], v[2], v[3]))
                                    : decode(v, kind);
}
static std::optional<Vector> encodeVelocity(const QVariant& v, ValueKind kind) {
    if (kind != ValueKind::Color) return encode(v, kind);
    if (! v.canConvert<QVector4D>()) return {};
    const auto q = v.value<QVector4D>();
    Vector     result { q.x(), q.y(), q.z(), q.w() };
    return finite(result) ? std::optional(result) : std::nullopt;
}
} // namespace anim
MotionValueBase::MotionValueBase(anim::ValueKind kind, QObject* p)
    : QObject(p),
      m_kind(kind),
      m_runner(kind == anim::ValueKind::Real    ? 1
               : kind == anim::ValueKind::Color ? 4
                                                : 2,
               this) {
    connect(&m_runner, &MotionRunner::changed, this, &MotionValueBase::changed);
    connect(&m_runner,
            &MotionRunner::ended,
            this,
            [this](QString              id,
                   MotionRunner::Reason reason,
                   const QVariantList&  v,
                   const QVariantList&  velocity) {
                anim::Vector values, velocities;
                for (const auto& x : v) values.append(x.toDouble());
                for (const auto& x : velocity) velocities.append(x.toDouble());
                emit ended(id,
                           reason,
                           anim::decode(values, m_kind),
                           anim::velocityValue(velocities, m_kind));
            });
}
void MotionValueBase::componentComplete() {
    m_initializing = false;
    if (! m_settings && qmlEngine(this))
        setSettings(qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(this, true))->motion());
}
AnimationSpec MotionValueBase::spec() const {
    return m_spec.value_or(m_settings ? (m_kind == anim::ValueKind::Color ? m_settings->effects()
                                                                          : m_settings->spatial())
                                      : Motion::spring());
}
void MotionValueBase::setSpec(AnimationSpec v) {
    if (! v.valid() || m_spec == v) return;
    m_spec = v;
    const QPointer<MotionValueBase> guard(this);
    emit                            specChanged();
    if (guard) policyChanged();
}
void MotionValueBase::resetSpec() {
    if (! m_spec) return;
    m_spec.reset();
    const QPointer<MotionValueBase> guard(this);
    emit                            specChanged();
    if (guard) policyChanged();
}
void MotionValueBase::setManualClock(bool v) {
    if (v == manualClock()) return;
    m_runner.setManualClock(v);
    emit manualClockChanged();
}
void MotionValueBase::setSettings(MotionSettings* v) {
    if (m_settings == v) return;
    disconnect(m_policyChanged);
    disconnect(m_policyDestroyed);
    m_settings = v;
    if (v) {
        m_policyChanged =
            connect(v, &MotionSettings::changed, this, &MotionValueBase::refreshPolicy);
        m_policyDestroyed = connect(v, &QObject::destroyed, this, [this] {
            m_settings = nullptr;
            refreshPolicy();
        });
    }
    const QPointer<MotionValueBase> guard(this);
    refreshPolicy();
    if (guard) emit settingsChanged();
}
void MotionValueBase::refreshPolicy() {
    const QPointer<MotionValueBase> guard(this);
    m_runner.setDurationScale(
        m_settings ? (m_settings->reducedMotion() ? 0 : m_settings->durationScale()) : 1);
    if (! guard) return;
    emit specChanged();
    if (guard) policyChanged();
}
void     MotionValueBase::policyChanged() {}
QVariant MotionValueBase::value() const { return anim::decode(m_runner.sample().value, m_kind); }
QVariant MotionValueBase::velocity() const {
    return anim::velocityValue(m_runner.sample().velocity, m_kind);
}
QVariant MotionValueBase::target() const { return anim::decode(m_runner.target(), m_kind); }
bool     MotionValueBase::setInitial(const QVariant& v) { return m_initializing && snap(v); }
QString  MotionValueBase::animate(const QVariant& v, AnimationSpec spec, const QVariant& velocity) {
    const auto target = anim::encode(v, m_kind);
    auto       speed =
        velocity.isValid() ? anim::encodeVelocity(velocity, m_kind) : std::optional<anim::Vector>();
    if (! target || (velocity.isValid() && ! speed)) return {};
    m_initializing = false;
    return m_runner.animateTo(*target, spec, speed);
}
bool MotionValueBase::snap(const QVariant& v) {
    const auto target = anim::encode(v, m_kind);
    return target && m_runner.snapTo(*target);
}
bool MotionValueBase::update(const QString& id, const QVariant& v) {
    const auto target = anim::encode(v, m_kind);
    return target && m_runner.updateInteraction(id, *target);
}
QString MotionValueBase::settle(const QString& id, const QVariant& v, const QVariant& velocity,
                                AnimationSpec spec) {
    const auto target = anim::encode(v, m_kind), speed = anim::encodeVelocity(velocity, m_kind);
    return target && speed ? m_runner.settleInteraction(id, *target, *speed, spec) : QString();
}
void AnimatedValueBase::follow(const QVariant& value) {
    if (! anim::encode(value, m_kind) || value == m_requested) return;
    m_requested = value;
    const QPointer<AnimatedValueBase> guard(this);
    if (m_initializing || ! m_enabled)
        snap(value);
    else
        animate(value, spec());
    if (guard) emit targetValueChanged();
}
void AnimatedValueBase::setEnabled(bool v) {
    if (v == m_enabled) return;
    m_enabled = v;
    const QPointer<AnimatedValueBase> guard(this);
    if (! v) snap(requested());
    if (guard) emit enabledChanged();
}
void AnimatedValueBase::componentComplete() {
    const QPointer<AnimatedValueBase> guard(this);
    MotionValueBase::componentComplete();
    if (guard) snap(requested());
}
void AnimatedValueBase::policyChanged() {
    if (! m_initializing && m_enabled && m_requested.isValid() && ! paused())
        animate(m_requested, spec());
}
} // namespace qml_material
