#include "qml_material/anim/seekable_transition.hpp"
#include "qml_material/style/theme.hpp"
#include <cmath>
namespace qml_material
{
TransitionTrack::TransitionTrack(anim::ValueKind kind, QObject* p): QObject(p), m_kind(kind) {
    const int n = kind == anim::ValueKind::Real ? 1 : kind == anim::ValueKind::Color ? 4 : 2;
    m_sample    = { anim::Vector(n, 0), anim::Vector(n, 0) };
}
void TransitionTrack::setValues(QVariantMap v) {
    if (v == m_values) return;
    m_values = std::move(v);
    emit definitionChanged();
}
void TransitionTrack::setSpec(AnimationSpec v) {
    if (! v.valid() || v == m_spec) return;
    m_spec = v;
    emit definitionChanged();
}
SeekableTransition::SeekableTransition(QObject* p): QObject(p) {
    connect(&m_driver, &MotionRunner::changed, this, &SeekableTransition::sample);
    connect(&m_driver,
            &MotionRunner::ended,
            this,
            [this](QString id, MotionRunner::Reason reason, QVariantList, QVariantList) {
                if (! m_runs.contains(id)) return;
                const auto                         state = m_runs.take(id);
                const QPointer<SeekableTransition> guard(this);
                if (id == m_run) {
                    m_run.clear();
                    m_active = false;
                    if (reason == MotionRunner::Finished || reason == MotionRunner::Completed)
                        m_current = state;
                    emit changed();
                }
                if (guard) emit ended(id, reason, state);
            });
}
SeekableTransition::~SeekableTransition() {
    for (auto t : m_tracks)
        if (t) {
            disconnect(t, nullptr, this, nullptr);
            t->m_owner = nullptr;
        }
}
QQmlListProperty<TransitionTrack> SeekableTransition::tracks() {
    return { this,
             this,
             [](QQmlListProperty<TransitionTrack>* p, TransitionTrack* t) {
                 static_cast<SeekableTransition*>(p->data)->addTrack(t);
             },
             [](QQmlListProperty<TransitionTrack>* p) -> qsizetype {
                 return static_cast<SeekableTransition*>(p->data)->m_tracks.size();
             },
             [](QQmlListProperty<TransitionTrack>* p, qsizetype i) -> TransitionTrack* {
                 return static_cast<SeekableTransition*>(p->data)->m_tracks.value(i);
             },
             [](QQmlListProperty<TransitionTrack>* p) {
                 auto*                              s = static_cast<SeekableTransition*>(p->data);
                 const QPointer<SeekableTransition> guard(s);
                 s->stop();
                 if (! guard) return;
                 for (auto t : s->m_tracks)
                     if (t) {
                         disconnect(t, nullptr, s, nullptr);
                         t->m_owner = nullptr;
                     }
                 s->m_tracks.clear();
                 s->m_prepared.clear();
             } };
}
bool SeekableTransition::addTrack(TransitionTrack* t) {
    if (! t || t->m_owner || m_active) return false;
    t->m_owner = this;
    m_tracks.append(t);
    connect(t, &TransitionTrack::definitionChanged, this, [this] {
        if (! m_active) initialize();
    });
    connect(t, &QObject::destroyed, this, [this] {
        m_tracks.removeAll(nullptr);
        stop();
    });
    if (! m_initializing) initialize();
    return true;
}
void SeekableTransition::setInitialState(QString s) {
    if (m_active || m_initial == s) return;
    m_initial = m_current = m_target = s;
    const QPointer<SeekableTransition> guard(this);
    initialize();
    if (guard) emit initialStateChanged();
}
void SeekableTransition::componentComplete() {
    m_initializing = false;
    const QPointer<SeekableTransition> guard(this);
    if (! m_settings && qmlEngine(this))
        setSettings(qobject_cast<Theme*>(qmlAttachedPropertiesObject<Theme>(this, true))->motion());
    if (guard) initialize();
}
void SeekableTransition::initialize() {
    if (m_initializing || m_active) return;
    const QPointer<SeekableTransition> guard(this);
    const auto                         tracks = m_tracks;
    for (auto t : tracks)
        if (t) {
            const auto v = anim::encode(t->m_values.value(m_current), t->kind());
            if (v) t->m_sample = { *v, anim::Vector(v->size(), 0) };
        }
    for (auto t : tracks) {
        if (! guard) return;
        if (t) emit t->valueChanged();
    }
}
bool SeekableTransition::prepare(const QString& state) {
    QList<Track> prepared;
    int          duration = 0;
    for (auto t : m_tracks) {
        if (! t) return false;
        const auto target = anim::encode(t->m_values.value(state), t->kind());
        if (! target) return false;
        auto motion = anim::PreparedMotion::prepare(
            t->m_sample.value, *target, t->m_sample.velocity, t->spec());
        if (! motion) return false;
        duration = std::max(duration, motion->duration());
        prepared.append({ t, *motion });
    }
    m_prepared = std::move(prepared);
    m_duration = duration;
    m_origin   = m_current;
    m_target   = state;
    m_fraction = 0;
    return true;
}
QString SeekableTransition::animateTo(QString state) {
    if (m_initializing || m_driver.state() == MotionRunner::Interacting || ! prepare(state))
        return {};
    m_configuring = true;
    m_driver.snapTo({ 0 });
    m_active      = true;
    const auto id = m_driver.animateTo({ 1 }, Motion::tween(m_duration));
    m_run         = id;
    m_runs.insert(id, state);
    m_configuring = false;
    sample();
    return id;
}
bool SeekableTransition::snapTo(QString state) {
    for (auto t : m_tracks)
        if (! t || ! anim::encode(t->m_values.value(state), t->kind())) return false;
    m_configuring = true;
    if (m_driver.state() == MotionRunner::Interacting)
        m_driver.cancelInteraction(m_driver.id());
    else
        m_driver.stop();
    m_active = false;
    m_run.clear();
    m_prepared.clear();
    m_current = m_target = state;
    m_fraction           = 0;
    m_configuring        = false;
    const QPointer<SeekableTransition> guard(this);
    initialize();
    if (guard) emit changed();
    return true;
}
QString SeekableTransition::beginSeek(QString state) {
    if (m_initializing || m_driver.state() == MotionRunner::Interacting) return {};
    if ((! m_active || state != m_target) && ! prepare(state)) return {};
    m_configuring = true;
    m_driver.stop();
    m_driver.snapTo({ m_fraction });
    const auto id = m_driver.beginInteraction();
    m_run.clear();
    m_active      = true;
    m_configuring = false;
    sample();
    return id;
}
bool SeekableTransition::seek(QString id, qreal fraction) {
    if (! std::isfinite(fraction) || fraction < 0 || fraction > 1) return false;
    return m_driver.updateInteraction(id, { fraction });
}
QString SeekableTransition::settle(QString id, Endpoint endpoint, qreal velocity,
                                   AnimationSpec spec) {
    if (endpoint != Origin && endpoint != Destination) return {};
    m_configuring  = true;
    const auto run = m_driver.settleInteraction(
        id, { endpoint == Destination ? qreal(1) : qreal(0) }, { velocity }, spec);
    if (! run.isEmpty()) {
        m_run = run;
        m_runs.insert(run, endpoint == Destination ? m_target : m_origin);
    }
    m_configuring = false;
    if (! run.isEmpty()) sample();
    return run;
}
void SeekableTransition::stop() {
    m_configuring = true;
    if (m_driver.state() == MotionRunner::Interacting)
        m_driver.cancelInteraction(m_driver.id());
    else
        m_driver.stop();
    m_active = false;
    m_run.clear();
    m_configuring = false;
    emit changed();
}
void SeekableTransition::sample() {
    if (m_configuring) return;
    m_fraction          = std::clamp(m_driver.sample().value[0], qreal(0), qreal(1));
    const auto prepared = m_prepared;
    for (const auto& t : prepared)
        if (t.track) {
            t.track->m_sample = t.motion.sample(m_fraction * m_duration);
            const auto rate   = m_driver.state() == MotionRunner::Interacting
                                    ? 0
                                    : m_driver.sample().velocity[0] * m_duration / 1000;
            for (auto& v : t.track->m_sample.velocity) v *= rate;
        }
    const QPointer<SeekableTransition> guard(this);
    for (const auto& t : prepared) {
        if (! guard) return;
        if (t.track) emit t.track->valueChanged();
    }
    if (guard) emit changed();
}
void SeekableTransition::setSettings(MotionSettings* s) {
    if (m_settings == s) return;
    disconnect(m_policy);
    disconnect(m_policyDestroyed);
    m_settings = s;
    if (s) {
        m_policy = connect(s, &MotionSettings::changed, this, &SeekableTransition::refreshPolicy);
        m_policyDestroyed = connect(s, &QObject::destroyed, this, [this] {
            m_settings = nullptr;
            refreshPolicy();
        });
    }
    const QPointer<SeekableTransition> guard(this);
    refreshPolicy();
    if (guard) emit settingsChanged();
}
void SeekableTransition::refreshPolicy() {
    m_driver.setDurationScale(
        m_settings ? (m_settings->reducedMotion() ? 0 : m_settings->durationScale()) : 1);
}
} // namespace qml_material
