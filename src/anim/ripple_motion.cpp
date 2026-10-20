#include "qml_material/anim/ripple_motion.hpp"
#include "qml_material/util/qt.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace qml_material
{
class RippleClock : public QAbstractAnimation {
public:
    explicit RippleClock(RippleMotion* owner): QAbstractAnimation(owner), m_owner(owner) {
        setObjectName(QStringLiteral("rippleClock"));
    }
    int duration() const override { return -1; }

private:
    void          updateCurrentTime(int time) override { m_owner->tick(time); }
    RippleMotion* m_owner;
};
namespace
{
qreal outCubic(qreal t) { return 1 - std::pow(1 - std::clamp(t, qreal(0), qreal(1)), 3); }
bool  validOpacity(qreal v) { return std::isfinite(v) && v >= 0 && v <= 1; }
} // namespace
RippleMotion::RippleMotion(QObject* parent)
    : QObject(parent),
      m_clock(std::make_unique<RippleClock>(this)),
      m_defaultIndication(this, [this] {
          QPointer<RippleMotion> guard(this);
          updateTarget();
          if (guard) Q_EMIT inputChanged();
      }) {}
RippleMotion::~RippleMotion() {
    m_clock->stop();
    utils::disconnectAll(m_sourceConnections);
}
int  RippleMotion::now() const { return m_clock->currentTime(); }
bool RippleMotion::active() const {
    return m_complete && m_enabled &&
           (! m_hasTarget || (m_target && m_activity && m_activity->active()));
}
void RippleMotion::componentComplete() {
    m_complete = true;
    updateTarget(true);
}
void RippleMotion::setInteractionSource(InteractionSource* source) {
    if (m_source == source) return;
    utils::disconnectAll(m_sourceConnections);
    m_source = source;
    QPointer<RippleMotion> guard(this);
    reset();
    if (! guard || m_source != source) return;
    if (source) {
        m_sourceConnections.append(connect(
            source, &InteractionSource::interaction, this, [this](const Interaction& event) {
                if (event.kind != Interaction::Press) return;
                if (event.phase == Interaction::Begin) {
                    if (const auto position =
                            event.positionFor(m_target ? m_target.data() : event.originItem()))
                        startPress(event.id, *position);
                } else
                    endPress(event.id, event.phase);
            }));
        m_sourceConnections.append(connect(source, &QObject::destroyed, this, [this] {
            m_source = nullptr;
            QPointer<RippleMotion> guard(this);
            reset();
            if (guard) updateTarget(true);
        }));
    }
    m_defaultIndication.setSource(m_indication ? nullptr : source);
    if (! guard || m_source != source) return;
    updateTarget(true);
    if (guard) Q_EMIT inputChanged();
}
IndicationValues RippleMotion::indication() const {
    if (m_indication) return *m_indication;
    auto values         = m_defaultIndication.values();
    values.color        = m_color;
    values.pressOpacity = m_pressOpacity;
    return values;
}
void RippleMotion::setIndication(const IndicationValues& indication) {
    if (m_indication && *m_indication == indication) return;
    m_indication = indication;
    QPointer<RippleMotion> guard(this);
    m_defaultIndication.setSource(nullptr);
    if (! guard) return;
    updateTarget();
    if (guard) Q_EMIT inputChanged();
}
void RippleMotion::resetIndication() {
    if (! m_indication) return;
    m_indication.reset();
    QPointer<RippleMotion> guard(this);
    m_defaultIndication.setSource(m_source);
    if (! guard) return;
    updateTarget();
    if (guard) Q_EMIT inputChanged();
}
void RippleMotion::setTarget(QQuickItem* target) {
    if (m_target == target && m_hasTarget == bool(target)) return;
    disconnect(m_targetDestroyed);
    m_activity.reset();
    m_target    = target;
    m_hasTarget = bool(target);
    if (target) {
        m_activity = std::make_unique<ItemActivity>(target);
        connect(m_activity.get(), &ItemActivity::changed, this, [this] {
            updateTarget(true);
        });
        m_targetDestroyed = connect(target, &QObject::destroyed, this, [this] {
            m_target = nullptr;
            reset();
        });
    }
    QPointer<RippleMotion> guard(this);
    reset();
    if (! guard) return;
    updateTarget(true);
    if (guard) Q_EMIT inputChanged();
}
void RippleMotion::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    QPointer<RippleMotion> guard(this);
    reset();
    if (! guard) return;
    updateTarget(true);
    if (guard) Q_EMIT inputChanged();
}
void RippleMotion::setStateOpacity(qreal value) {
    if (! validOpacity(value) || value == m_stateOpacity) return;
    m_stateOpacity = value;
    QPointer<RippleMotion> guard(this);
    updateTarget();
    if (guard) Q_EMIT inputChanged();
}
void RippleMotion::setPressOpacity(qreal value) {
    if (! validOpacity(value) || value == m_pressOpacity) return;
    m_pressOpacity = value;
    Q_EMIT inputChanged();
}
void RippleMotion::setColor(const QColor& color) {
    if (color == m_color) return;
    m_color = color;
    QPointer<RippleMotion> guard(this);
    updateTarget();
    if (guard) Q_EMIT inputChanged();
}
void RippleMotion::updateTarget(bool snap) {
    IndicationValues values;
    values.color     = m_color;
    values.opacity   = m_stateOpacity;
    values.available = true;
    if (m_source) values = indication();
    const auto                   time = now();
    const QPointer<RippleMotion> guard(this);
    const auto                   revision = m_revision;
    if (m_running) tick(time);
    if (! guard || revision != m_revision) return;
    const auto target       = values.available && validOpacity(values.opacity) ? values.opacity : 0;
    const bool changed      = target != m_toOpacity;
    const int  previousKind = m_kind;
    m_kind                  = values.kind;
    if (values.color.isValid()) m_ink = values.color;
    if (m_wave && (! validOpacity(values.pressOpacity) || values.pressOpacity == 0)) {
        m_wave          = false;
        m_rippleOpacity = 0;
    } else if (m_wave) {
        const auto fade = m_waveAlpha > 0 ? m_rippleOpacity / m_waveAlpha : 0;
        m_waveAlpha     = values.pressOpacity;
        m_rippleOpacity = fade * m_waveAlpha;
    }
    if (! active() || ! values.available || snap) {
        m_fromOpacity = m_opacity = m_toOpacity = target;
        m_stateDuration                         = 0;
        if (! active() || ! values.available) {
            m_wave          = false;
            m_pressId       = 0;
            m_rippleOpacity = 0;
        }
    } else if (changed) {
        m_fromOpacity   = m_opacity;
        m_toOpacity     = target;
        m_stateStart    = time;
        const int kind  = m_kind != Interaction::None ? m_kind : previousKind;
        m_stateDuration = kind == Interaction::Drag              ? (m_kind ? 45 : 150)
                          : kind == Interaction::Focus && m_kind ? 45
                                                                 : 15;
    }
    ++m_revision;
    Q_EMIT valueChanged();
    if (guard) schedule();
}
void RippleMotion::startPress(quint64 id, const QPointF& point) {
    if (! id || ! active() || ! m_source) return;
    const auto values = indication();
    if (! values.available) return;
    const qreal alpha = values.pressOpacity;
    if (! validOpacity(alpha) || alpha == 0) return;
    m_pressId    = id;
    m_wave       = true;
    m_releasing  = false;
    m_pressStart = now();
    m_progress = m_phase = 0;
    m_waveAlpha = m_rippleOpacity = alpha;
    m_origin                      = point;
    ++m_revision;
    QPointer<RippleMotion> guard(this);
    Q_EMIT valueChanged();
    if (guard) schedule();
}
void RippleMotion::endPress(quint64 id, Interaction::Phase) {
    if (! id || id != m_pressId || ! m_wave || m_releasing) return;
    QPointer<RippleMotion> guard(this);
    tick(now());
    if (! guard || id != m_pressId) return;
    m_releasing    = true;
    m_releaseStart = now();
    m_releaseFrom  = m_progress;
    ++m_revision;
    schedule();
}
void RippleMotion::reset() {
    ++m_revision;
    m_wave = m_releasing = false;
    m_pressId            = 0;
    m_rippleOpacity = m_progress = m_phase = 0;
    m_stateDuration                        = 0;
    m_clock->stop();
    const bool             running  = std::exchange(m_running, false);
    const auto             revision = m_revision;
    QPointer<RippleMotion> guard(this);
    Q_EMIT valueChanged();
    if (guard && running && revision == m_revision && ! m_running) Q_EMIT runningChanged();
}
void RippleMotion::tick(int time) {
    if (m_stateDuration) {
        const qreal t =
            std::clamp(qreal(time - m_stateStart) / m_stateDuration, qreal(0), qreal(1));
        m_opacity = m_fromOpacity + (m_toOpacity - m_fromOpacity) * t;
        if (t >= 1) m_stateDuration = 0;
    }
    if (m_wave) {
        m_phase = ((time - m_pressStart) % 7000) / 214.0;
        if (m_releasing) {
            const int elapsed = time - m_releaseStart;
            m_progress        = m_releaseFrom + (1 - m_releaseFrom) * outCubic(elapsed / 350.0);
            m_rippleOpacity   = m_waveAlpha * (1 - std::clamp((elapsed - 150) / 150.0, 0.0, 1.0));
            if (elapsed >= 350) {
                m_wave          = false;
                m_rippleOpacity = 0;
            }
        } else {
            m_progress = .4 * outCubic((time - m_pressStart) / 450.0);
        }
    }
    const auto             revision = m_revision;
    QPointer<RippleMotion> guard(this);
    Q_EMIT valueChanged();
    if (guard && revision == m_revision) schedule();
}
void RippleMotion::schedule() {
    const bool running = active() && (m_wave || m_stateDuration);
    if (m_running == running) return;
    m_running = running;
    QPointer<RippleMotion> guard(this);
    if (running) {
        // QAbstractAnimation restarts its clock at zero.
        const int elapsed = now();
        m_stateStart -= elapsed;
        m_pressStart -= elapsed;
        m_releaseStart -= elapsed;
        m_clock->start();
    } else
        m_clock->stop();
    if (guard && m_running == running) Q_EMIT runningChanged();
}
} // namespace qml_material
