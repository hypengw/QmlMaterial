#include "qml_material/anim/size_transition.hpp"
#include "qml_material/token/token.hpp"
#include <QPointer>
#include <cmath>

namespace qml_material
{
SizeTransition::SizeTransition(QObject* parent)
    : QObject(parent),
      m_animation(this),
      m_duration(qRound(token::Duration {}.long1)),
      m_easing(token::Easing {}.emphasized()) {
    connect(&m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        if (! m_configuring) setSize(bounded(value.toSizeF()));
    });
    connect(&m_animation, &QVariantAnimation::finished, this, [this] {
        if (m_animation.state() == QAbstractAnimation::Stopped && m_size == bounded(m_target))
            setRunning(false);
    });
}
QSizeF SizeTransition::bounded(const QSizeF& size) const {
    return {
        std::max<qreal>(
            0, m_maximum.width() < 0 ? size.width() : std::min(size.width(), m_maximum.width())),
        std::max<qreal>(
            0, m_maximum.height() < 0 ? size.height() : std::min(size.height(), m_maximum.height()))
    };
}
void SizeTransition::setSize(const QSizeF& size) {
    if (m_size == size) return;
    m_size = size;
    Q_EMIT sizeChanged();
}
void SizeTransition::setRunning(bool running) {
    if (m_running == running) return;
    m_running = running;
    Q_EMIT runningChanged();
}
void SizeTransition::setTargetSize(const QSizeF& size) {
    if (! std::isfinite(size.width()) || ! std::isfinite(size.height()) || size.width() < 0 ||
        size.height() < 0 || (m_hasTarget && m_target == size))
        return;
    m_hasTarget = true;
    m_target    = size;
    QPointer<SizeTransition> guard(this);
    Q_EMIT targetSizeChanged();
    if (guard) retarget();
}
void SizeTransition::setMaximumSize(const QSizeF& size) {
    if (! std::isfinite(size.width()) || ! std::isfinite(size.height()) || size.width() < -1 ||
        size.height() < -1 || m_maximum == size)
        return;
    m_maximum = size;
    QPointer<SizeTransition> guard(this);
    Q_EMIT maximumSizeChanged();
    if (guard) retarget();
}
void SizeTransition::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    QPointer<SizeTransition> guard(this);
    Q_EMIT enabledChanged();
    if (guard) retarget();
}
void SizeTransition::setDuration(int duration) {
    if (duration < 0 || m_duration == duration) return;
    m_duration = duration;
    QPointer<SizeTransition> guard(this);
    Q_EMIT durationChanged();
    if (guard) retarget(true);
}
void SizeTransition::setEasing(const QEasingCurve& easing) {
    if (m_easing == easing) return;
    m_easing = easing;
    QPointer<SizeTransition> guard(this);
    Q_EMIT easingChanged();
    if (guard) retarget(true);
}
void SizeTransition::retarget(bool restart) {
    const auto               revision = ++m_revision;
    QPointer<SizeTransition> guard(this);
    setSize(bounded(m_size));
    if (! guard || m_revision != revision) return;
    const auto target = bounded(m_target);
    if (! m_complete || ! m_initialized || ! m_enabled || duration() == 0 || m_size == target) {
        complete();
        if (guard) m_initialized = m_complete && m_hasTarget;
        return;
    }
    if (! restart && m_running && m_animation.endValue().toSizeF() == target) return;
    m_configuring = true;
    m_animation.stop();
    m_animation.setDuration(m_duration);
    m_animation.setEasingCurve(m_easing);
    m_animation.setStartValue(m_size);
    m_animation.setEndValue(target);
    m_animation.setCurrentTime(0);
    m_configuring = false;
    m_animation.start();
    if (guard && m_revision == revision) setRunning(true);
}
void SizeTransition::complete() {
    const auto revision = ++m_revision;
    m_animation.stop();
    QPointer<SizeTransition> guard(this);
    setSize(bounded(m_target));
    if (guard && m_revision == revision) setRunning(false);
}
void SizeTransition::classBegin() { m_complete = false; }
void SizeTransition::componentComplete() {
    m_complete = true;
    QPointer<SizeTransition> guard(this);
    complete();
    if (guard) m_initialized = true;
}
} // namespace qml_material
