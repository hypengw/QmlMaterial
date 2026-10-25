#pragma once

#include "qml_material/token/motion.hpp"
#include <QEasingCurve>
#include <QList>
#include <optional>

namespace qml_material
{
struct QML_MATERIAL_API AnimationSpec {
    Q_GADGET
    QML_VALUE_TYPE(animationSpec)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(Kind kind MEMBER kind FINAL)
    Q_PROPERTY(int duration MEMBER duration FINAL)
    Q_PROPERTY(int delay MEMBER delay FINAL)
    Q_PROPERTY(QEasingCurve easing MEMBER easing FINAL)
    Q_PROPERTY(qreal stiffness MEMBER stiffness FINAL)
    Q_PROPERTY(qreal dampingRatio MEMBER dampingRatio FINAL)
    Q_PROPERTY(qreal threshold MEMBER threshold FINAL)
    Q_PROPERTY(qreal velocityThreshold MEMBER velocityThreshold FINAL)
public:
    enum Kind
    {
        Tween,
        Spring,
        Snap
    };
    Q_ENUM(Kind)
    Kind         kind     = Spring;
    int          duration = 300, delay = 0;
    QEasingCurve easing { QEasingCurve::Linear };
    qreal        stiffness = 700, dampingRatio = .9, threshold = .01, velocityThreshold = .1;
    bool         valid() const;
    bool         operator==(const AnimationSpec&) const = default;
};
struct QML_MATERIAL_API DecaySpec {
    Q_GADGET
    QML_VALUE_TYPE(decaySpec)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(qreal friction MEMBER friction FINAL)
    Q_PROPERTY(qreal velocityThreshold MEMBER velocityThreshold FINAL)
public:
    qreal friction = 5, velocityThreshold = .1;
    bool  valid() const;
    bool  operator==(const DecaySpec&) const = default;
};
class QML_MATERIAL_API Motion : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    using QObject::QObject;
    Q_INVOKABLE static AnimationSpec tween(int duration, QEasingCurve easing = QEasingCurve(),
                                           int delay = 0);
    Q_INVOKABLE static AnimationSpec spring(qreal stiffness = 700, qreal dampingRatio = .9);
    Q_INVOKABLE static AnimationSpec snap();
    Q_INVOKABLE static DecaySpec     decay(qreal friction = 5, qreal velocityThreshold = .1);
};
namespace anim
{
using Vector = QList<qreal>;
struct Sample {
    Vector value, velocity;
};
struct SpringSample {
    qreal value = 0, velocity = 0;
};
QML_MATERIAL_API SpringSample sampleSpring(token::SpringSpec, qreal, qreal, qreal, qreal);
QML_MATERIAL_API int          springDuration(token::SpringSpec, qreal, qreal, qreal, qreal);
QML_MATERIAL_API bool         finite(const Vector&);
class QML_MATERIAL_API PreparedMotion {
public:
    static std::optional<PreparedMotion> prepare(const Vector& from, const Vector& to,
                                                 const Vector& velocity, AnimationSpec,
                                                 const Vector& thresholds = {});
    static std::optional<PreparedMotion> decay(const Vector& from, const Vector& velocity,
                                               DecaySpec);
    Sample                               sample(qreal milliseconds) const;
    const Vector&                        target() const { return m_to; }
    int                                  duration() const { return m_duration; }

private:
    Vector                   m_from, m_to, m_velocity;
    QList<int>               m_durations;
    AnimationSpec            m_spec;
    std::optional<DecaySpec> m_decay;
    int                      m_duration = 0;
};
} // namespace anim
} // namespace qml_material
