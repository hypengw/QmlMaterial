#pragma once
#include "qml_material/anim/motion_runner.hpp"
#include "qml_material/style/motion_settings.hpp"
#include <QQmlParserStatus>
#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QVector4D>

namespace qml_material
{
namespace anim
{
enum class ValueKind
{
    Real,
    Point,
    Size,
    Color
};
QML_MATERIAL_API std::optional<Vector> encode(const QVariant&, ValueKind);
QML_MATERIAL_API QVariant              decode(const Vector&, ValueKind);
QML_MATERIAL_API QVariant              velocityValue(const Vector&, ValueKind);
} // namespace anim
class QML_MATERIAL_API MotionValueBase : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ANONYMOUS
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(AnimationSpec spec READ spec WRITE setSpec RESET resetSpec NOTIFY specChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY changed FINAL)
    Q_PROPERTY(bool running READ running NOTIFY changed FINAL)
    Q_PROPERTY(bool paused READ paused NOTIFY changed FINAL)
    Q_PROPERTY(int state READ state NOTIFY changed FINAL)
    Q_PROPERTY(QString runId READ runId NOTIFY changed FINAL)
    Q_PROPERTY(
        bool manualClock READ manualClock WRITE setManualClock NOTIFY manualClockChanged FINAL)
    Q_PROPERTY(MotionSettings* motion READ settings WRITE setSettings NOTIFY settingsChanged FINAL)
public:
    enum State
    {
        Idle        = MotionRunner::Idle,
        Running     = MotionRunner::Running,
        Paused      = MotionRunner::Paused,
        Interacting = MotionRunner::Interacting
    };
    Q_ENUM(State)
    enum Reason
    {
        Finished     = MotionRunner::Finished,
        Completed    = MotionRunner::Completed,
        Cancelled    = MotionRunner::Cancelled,
        Interrupted  = MotionRunner::Interrupted,
        BoundReached = MotionRunner::BoundReached
    };
    Q_ENUM(Reason)
    void             classBegin() override { m_initializing = true; }
    void             componentComplete() override;
    AnimationSpec    spec() const;
    void             setSpec(AnimationSpec);
    void             resetSpec();
    bool             active() const { return m_runner.active(); }
    bool             running() const { return m_runner.state() == MotionRunner::Running; }
    bool             paused() const { return m_runner.state() == MotionRunner::Paused; }
    int              state() const { return m_runner.state(); }
    QString          runId() const { return m_runner.id(); }
    bool             manualClock() const { return m_runner.manualClock(); }
    void             setManualClock(bool);
    MotionSettings*  settings() const { return m_settings; }
    void             setSettings(MotionSettings*);
    MotionRunner&    runner() { return m_runner; }
    Q_INVOKABLE void advance(qreal milliseconds) { m_runner.advance(milliseconds); }
    Q_SIGNAL void    changed();
    Q_SIGNAL void    specChanged();
    Q_SIGNAL void    manualClockChanged();
    Q_SIGNAL void    settingsChanged();
    Q_SIGNAL void    ended(QString id, int reason, QVariant value, QVariant velocity);

protected:
    MotionValueBase(anim::ValueKind, QObject*);
    QVariant              value() const;
    QVariant              velocity() const;
    QVariant              target() const;
    bool                  setInitial(const QVariant&);
    QString               animate(const QVariant&, AnimationSpec, const QVariant& velocity = {});
    bool                  snap(const QVariant&);
    bool                  update(const QString&, const QVariant&);
    QString               settle(const QString&, const QVariant&, const QVariant&, AnimationSpec);
    virtual void          policyChanged();
    const anim::ValueKind m_kind;
    MotionRunner          m_runner;
    bool                  m_initializing = true;

private:
    void                         refreshPolicy();
    QPointer<MotionSettings>     m_settings;
    QMetaObject::Connection      m_policyChanged, m_policyDestroyed;
    std::optional<AnimationSpec> m_spec;
};
class QML_MATERIAL_API AnimatableBase : public MotionValueBase {
    Q_OBJECT
    QML_ANONYMOUS
public:
    Q_INVOKABLE void    pause() { m_runner.pause(); }
    Q_INVOKABLE void    resume() { m_runner.resume(); }
    Q_INVOKABLE void    stop(QString id = {}) { m_runner.stop(id); }
    Q_INVOKABLE void    complete(QString id = {}) { m_runner.complete(id); }
    Q_INVOKABLE QString beginInteraction() { return m_runner.beginInteraction(); }
    Q_INVOKABLE bool    cancelInteraction(QString id) { return m_runner.cancelInteraction(id); }

protected:
    using MotionValueBase::MotionValueBase;
};
class QML_MATERIAL_API Animatable : public AnimatableBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal value READ value NOTIFY changed FINAL)
    Q_PROPERTY(qreal velocity READ velocity NOTIFY changed FINAL)
    Q_PROPERTY(qreal targetValue READ targetValue NOTIFY changed FINAL)
    Q_PROPERTY(
        qreal initialValue READ initialValue WRITE setInitialValue NOTIFY initialValueChanged FINAL)
public:
    explicit Animatable(QObject* parent = nullptr): AnimatableBase(anim::ValueKind::Real, parent) {}
    qreal value() const { return MotionValueBase::value().value<qreal>(); }
    qreal velocity() const { return MotionValueBase::velocity().value<qreal>(); }
    qreal targetValue() const { return target().value<qreal>(); }
    qreal initialValue() const { return m_initial; }
    void  setInitialValue(qreal v) {
        if (m_initial == v || ! m_initializing || ! anim::encode(QVariant::fromValue(v), m_kind))
            return;
        m_initial = v;
        const QPointer<Animatable> guard(this);
        if (setInitial(QVariant::fromValue(v)) && guard) emit initialValueChanged();
    }
    Q_INVOKABLE QString animateTo(qreal v) { return animate(QVariant::fromValue(v), spec()); }
    Q_INVOKABLE QString animateTo(qreal v, AnimationSpec s) {
        return animate(QVariant::fromValue(v), s);
    }
    Q_INVOKABLE QString animateTo(qreal v, AnimationSpec s, qreal velocity) {
        return animate(QVariant::fromValue(v), s, QVariant::fromValue(velocity));
    }
    Q_INVOKABLE bool snapTo(qreal v) { return snap(QVariant::fromValue(v)); }
    Q_INVOKABLE bool updateInteraction(QString id, qreal v) {
        return update(id, QVariant::fromValue(v));
    }
    Q_INVOKABLE QString settleInteraction(QString id, qreal v, qreal velocity, AnimationSpec s) {
        return settle(id, QVariant::fromValue(v), QVariant::fromValue(velocity), s);
    }
    Q_SIGNAL void initialValueChanged();

private:
    qreal m_initial { 0 };

public:
    Q_INVOKABLE QString animateDecay(qreal velocity, DecaySpec s) {
        const auto v = anim::encode(QVariant::fromValue(velocity), m_kind);
        return v ? m_runner.animateDecay(*v, s) : QString();
    }
    Q_INVOKABLE bool setBounds(qreal lower, qreal upper) {
        const auto lo = anim::encode(QVariant::fromValue(lower), m_kind),
                   hi = anim::encode(QVariant::fromValue(upper), m_kind);
        return lo && hi && m_runner.setBounds(*lo, *hi);
    }
};
class QML_MATERIAL_API PointAnimatable : public AnimatableBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QPointF value READ value NOTIFY changed FINAL)
    Q_PROPERTY(QPointF velocity READ velocity NOTIFY changed FINAL)
    Q_PROPERTY(QPointF targetValue READ targetValue NOTIFY changed FINAL)
    Q_PROPERTY(QPointF initialValue READ initialValue WRITE setInitialValue NOTIFY
                   initialValueChanged FINAL)
public:
    explicit PointAnimatable(QObject* parent = nullptr)
        : AnimatableBase(anim::ValueKind::Point, parent) {}
    QPointF value() const { return MotionValueBase::value().value<QPointF>(); }
    QPointF velocity() const { return MotionValueBase::velocity().value<QPointF>(); }
    QPointF targetValue() const { return target().value<QPointF>(); }
    QPointF initialValue() const { return m_initial; }
    void    setInitialValue(QPointF v) {
        if (m_initial == v || ! m_initializing || ! anim::encode(QVariant::fromValue(v), m_kind))
            return;
        m_initial = v;
        const QPointer<PointAnimatable> guard(this);
        if (setInitial(QVariant::fromValue(v)) && guard) emit initialValueChanged();
    }
    Q_INVOKABLE QString animateTo(QPointF v) { return animate(QVariant::fromValue(v), spec()); }
    Q_INVOKABLE QString animateTo(QPointF v, AnimationSpec s) {
        return animate(QVariant::fromValue(v), s);
    }
    Q_INVOKABLE QString animateTo(QPointF v, AnimationSpec s, QPointF velocity) {
        return animate(QVariant::fromValue(v), s, QVariant::fromValue(velocity));
    }
    Q_INVOKABLE bool snapTo(QPointF v) { return snap(QVariant::fromValue(v)); }
    Q_INVOKABLE bool updateInteraction(QString id, QPointF v) {
        return update(id, QVariant::fromValue(v));
    }
    Q_INVOKABLE QString settleInteraction(QString id, QPointF v, QPointF velocity,
                                          AnimationSpec s) {
        return settle(id, QVariant::fromValue(v), QVariant::fromValue(velocity), s);
    }
    Q_SIGNAL void initialValueChanged();

private:
    QPointF m_initial { QPointF() };

public:
    Q_INVOKABLE QString animateDecay(QPointF velocity, DecaySpec s) {
        const auto v = anim::encode(QVariant::fromValue(velocity), m_kind);
        return v ? m_runner.animateDecay(*v, s) : QString();
    }
    Q_INVOKABLE bool setBounds(QPointF lower, QPointF upper) {
        const auto lo = anim::encode(QVariant::fromValue(lower), m_kind),
                   hi = anim::encode(QVariant::fromValue(upper), m_kind);
        return lo && hi && m_runner.setBounds(*lo, *hi);
    }
};
class QML_MATERIAL_API SizeAnimatable : public AnimatableBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QSizeF value READ value NOTIFY changed FINAL)
    Q_PROPERTY(QSizeF velocity READ velocity NOTIFY changed FINAL)
    Q_PROPERTY(QSizeF targetValue READ targetValue NOTIFY changed FINAL)
    Q_PROPERTY(QSizeF initialValue READ initialValue WRITE setInitialValue NOTIFY
                   initialValueChanged FINAL)
public:
    explicit SizeAnimatable(QObject* parent = nullptr)
        : AnimatableBase(anim::ValueKind::Size, parent) {}
    QSizeF value() const { return MotionValueBase::value().value<QSizeF>(); }
    QSizeF velocity() const { return MotionValueBase::velocity().value<QSizeF>(); }
    QSizeF targetValue() const { return target().value<QSizeF>(); }
    QSizeF initialValue() const { return m_initial; }
    void   setInitialValue(QSizeF v) {
        if (m_initial == v || ! m_initializing || ! anim::encode(QVariant::fromValue(v), m_kind))
            return;
        m_initial = v;
        const QPointer<SizeAnimatable> guard(this);
        if (setInitial(QVariant::fromValue(v)) && guard) emit initialValueChanged();
    }
    Q_INVOKABLE QString animateTo(QSizeF v) { return animate(QVariant::fromValue(v), spec()); }
    Q_INVOKABLE QString animateTo(QSizeF v, AnimationSpec s) {
        return animate(QVariant::fromValue(v), s);
    }
    Q_INVOKABLE QString animateTo(QSizeF v, AnimationSpec s, QSizeF velocity) {
        return animate(QVariant::fromValue(v), s, QVariant::fromValue(velocity));
    }
    Q_INVOKABLE bool snapTo(QSizeF v) { return snap(QVariant::fromValue(v)); }
    Q_INVOKABLE bool updateInteraction(QString id, QSizeF v) {
        return update(id, QVariant::fromValue(v));
    }
    Q_INVOKABLE QString settleInteraction(QString id, QSizeF v, QSizeF velocity, AnimationSpec s) {
        return settle(id, QVariant::fromValue(v), QVariant::fromValue(velocity), s);
    }
    Q_SIGNAL void initialValueChanged();

private:
    QSizeF m_initial { QSizeF(0, 0) };

public:
    Q_INVOKABLE QString animateDecay(QSizeF velocity, DecaySpec s) {
        const auto v = anim::encode(QVariant::fromValue(velocity), m_kind);
        return v ? m_runner.animateDecay(*v, s) : QString();
    }
    Q_INVOKABLE bool setBounds(QSizeF lower, QSizeF upper) {
        const auto lo = anim::encode(QVariant::fromValue(lower), m_kind),
                   hi = anim::encode(QVariant::fromValue(upper), m_kind);
        return lo && hi && m_runner.setBounds(*lo, *hi);
    }
};
class QML_MATERIAL_API ColorAnimatable : public AnimatableBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor value READ value NOTIFY changed FINAL)
    Q_PROPERTY(QVector4D velocity READ velocity NOTIFY changed FINAL)
    Q_PROPERTY(QColor targetValue READ targetValue NOTIFY changed FINAL)
    Q_PROPERTY(QColor initialValue READ initialValue WRITE setInitialValue NOTIFY
                   initialValueChanged FINAL)
public:
    explicit ColorAnimatable(QObject* parent = nullptr)
        : AnimatableBase(anim::ValueKind::Color, parent) {}
    QColor    value() const { return MotionValueBase::value().value<QColor>(); }
    QVector4D velocity() const { return MotionValueBase::velocity().value<QVector4D>(); }
    QColor    targetValue() const { return target().value<QColor>(); }
    QColor    initialValue() const { return m_initial; }
    void      setInitialValue(QColor v) {
        if (m_initial == v || ! m_initializing || ! anim::encode(QVariant::fromValue(v), m_kind))
            return;
        m_initial = v;
        const QPointer<ColorAnimatable> guard(this);
        if (setInitial(QVariant::fromValue(v)) && guard) emit initialValueChanged();
    }
    Q_INVOKABLE QString animateTo(QColor v) { return animate(QVariant::fromValue(v), spec()); }
    Q_INVOKABLE QString animateTo(QColor v, AnimationSpec s) {
        return animate(QVariant::fromValue(v), s);
    }
    Q_INVOKABLE QString animateTo(QColor v, AnimationSpec s, QVector4D velocity) {
        return animate(QVariant::fromValue(v), s, QVariant::fromValue(velocity));
    }
    Q_INVOKABLE bool snapTo(QColor v) { return snap(QVariant::fromValue(v)); }
    Q_INVOKABLE bool updateInteraction(QString id, QColor v) {
        return update(id, QVariant::fromValue(v));
    }
    Q_INVOKABLE QString settleInteraction(QString id, QColor v, QVector4D velocity,
                                          AnimationSpec s) {
        return settle(id, QVariant::fromValue(v), QVariant::fromValue(velocity), s);
    }
    Q_SIGNAL void initialValueChanged();

private:
    QColor m_initial { QColor(Qt::transparent) };
};
class QML_MATERIAL_API AnimatedValueBase : public MotionValueBase {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
public:
    bool          enabled() const { return m_enabled; }
    void          setEnabled(bool);
    void          componentComplete() override;
    Q_SIGNAL void targetValueChanged();
    Q_SIGNAL void enabledChanged();

protected:
    using MotionValueBase::MotionValueBase;
    void     follow(const QVariant&);
    QVariant requested() const { return m_requested.isValid() ? m_requested : target(); }
    void     policyChanged() override;

private:
    QVariant m_requested;
    bool     m_enabled = true;
};
class QML_MATERIAL_API AnimatedReal : public AnimatedValueBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal value READ value NOTIFY changed FINAL)
    Q_PROPERTY(
        qreal targetValue READ targetValue WRITE setTargetValue NOTIFY targetValueChanged FINAL)
public:
    explicit AnimatedReal(QObject* p = nullptr): AnimatedValueBase(anim::ValueKind::Real, p) {}
    qreal value() const { return MotionValueBase::value().value<qreal>(); }
    qreal targetValue() const { return requested().value<qreal>(); }
    void  setTargetValue(qreal v) { follow(QVariant::fromValue(v)); }
};
class QML_MATERIAL_API AnimatedPoint : public AnimatedValueBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QPointF value READ value NOTIFY changed FINAL)
    Q_PROPERTY(
        QPointF targetValue READ targetValue WRITE setTargetValue NOTIFY targetValueChanged FINAL)
public:
    explicit AnimatedPoint(QObject* p = nullptr): AnimatedValueBase(anim::ValueKind::Point, p) {}
    QPointF value() const { return MotionValueBase::value().value<QPointF>(); }
    QPointF targetValue() const { return requested().value<QPointF>(); }
    void    setTargetValue(QPointF v) { follow(QVariant::fromValue(v)); }
};
class QML_MATERIAL_API AnimatedSize : public AnimatedValueBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QSizeF value READ value NOTIFY changed FINAL)
    Q_PROPERTY(
        QSizeF targetValue READ targetValue WRITE setTargetValue NOTIFY targetValueChanged FINAL)
public:
    explicit AnimatedSize(QObject* p = nullptr): AnimatedValueBase(anim::ValueKind::Size, p) {}
    QSizeF value() const { return MotionValueBase::value().value<QSizeF>(); }
    QSizeF targetValue() const { return requested().value<QSizeF>(); }
    void   setTargetValue(QSizeF v) { follow(QVariant::fromValue(v)); }
};
class QML_MATERIAL_API AnimatedColor : public AnimatedValueBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor value READ value NOTIFY changed FINAL)
    Q_PROPERTY(
        QColor targetValue READ targetValue WRITE setTargetValue NOTIFY targetValueChanged FINAL)
public:
    explicit AnimatedColor(QObject* p = nullptr): AnimatedValueBase(anim::ValueKind::Color, p) {}
    QColor value() const { return MotionValueBase::value().value<QColor>(); }
    QColor targetValue() const { return requested().value<QColor>(); }
    void   setTargetValue(QColor v) { follow(QVariant::fromValue(v)); }
};
} // namespace qml_material
