#pragma once

#include "qml_material/anim/motion_spec.hpp"
#include <QAbstractAnimation>
#include <QVariantMap>

namespace qml_material
{
class QML_MATERIAL_API MotionRunner : public QObject {
    Q_OBJECT
public:
    enum State
    {
        Idle,
        Running,
        Paused,
        Interacting
    };
    Q_ENUM(State)
    enum Reason
    {
        Finished,
        Completed,
        Cancelled,
        Interrupted,
        BoundReached
    };
    Q_ENUM(Reason)
    explicit MotionRunner(int dimensions, QObject* parent = nullptr);
    ~MotionRunner() override;
    State               state() const { return m_state; }
    bool                active() const { return m_state != Idle; }
    const anim::Sample& sample() const { return m_sample; }
    const anim::Vector& target() const { return m_target; }
    QString             id() const { return m_id; }
    qreal               elapsed() const { return m_elapsed; }
    int                 duration() const { return m_motion.duration(); }
    bool                manualClock() const { return m_manual; }
    void                setManualClock(bool);
    void                advance(qreal milliseconds);
    void                setPlayTime(qreal milliseconds);
    void                setDurationScale(qreal);
    qreal               durationScale() const { return m_scale; }
    bool                setBounds(anim::Vector lower, anim::Vector upper);
    QString animateTo(const anim::Vector&, AnimationSpec, std::optional<anim::Vector> velocity = {},
                      const anim::Vector& thresholds = {});
    QString animateDecay(const anim::Vector&, DecaySpec);
    bool    snapTo(const anim::Vector&);
    void    pause();
    void    resume();
    void    stop(const QString& expected = {});
    void    complete(const QString& expected = {});
    QString beginInteraction();
    bool    updateInteraction(const QString&, const anim::Vector&);
    bool    cancelInteraction(const QString&);
    QString settleInteraction(const QString&, const anim::Vector&, const anim::Vector&,
                              AnimationSpec);
    Q_SIGNAL void changed();
    Q_SIGNAL void ended(QString id, Reason reason, QVariantList value, QVariantList velocity);

private:
    class Clock;
    friend class Clock;
    QString              nextId();
    QString              start(anim::PreparedMotion);
    void                 terminate(Reason);
    void                 tick(qreal milliseconds);
    bool                 validVector(const anim::Vector&) const;
    bool                 clamp(anim::Vector&) const;
    void                 notify();
    Clock*               m_clock;
    anim::PreparedMotion m_motion;
    anim::Sample         m_sample;
    anim::Vector         m_target, m_lower, m_upper;
    State                m_state = Idle;
    QString              m_id;
    quint64              m_sequence = 0;
    qreal                m_elapsed = 0, m_scale = 1;
    bool                 m_manual = false;
};
} // namespace qml_material
