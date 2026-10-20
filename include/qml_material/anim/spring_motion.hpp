#pragma once

#include <QAbstractAnimation>
#include <QColor>
#include <QQmlParserStatus>
#include <memory>
#include "qml_material/token/motion.hpp"
#include "qml_material/util/corner.hpp"

namespace qml_material
{
namespace anim
{
struct SpringSample {
    qreal value    = 0;
    qreal velocity = 0;
};
QML_MATERIAL_API SpringSample sampleSpring(token::SpringSpec, qreal from, qreal to, qreal velocity,
                                           qreal seconds);
QML_MATERIAL_API int springDuration(token::SpringSpec, qreal from, qreal to, qreal velocity,
                                    qreal threshold);
} // namespace anim

class SpringClock;
/** @ingroup impl */
class QML_MATERIAL_API SpringMotionBase : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ANONYMOUS
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(
        qml_material::token::SpringSpec spec READ spec WRITE setSpec NOTIFY specChanged FINAL)
    Q_PROPERTY(qreal visibilityThreshold READ visibilityThreshold WRITE setVisibilityThreshold
                   NOTIFY visibilityThresholdChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged FINAL)
public:
    ~SpringMotionBase() override;
    void              classBegin() override;
    void              componentComplete() override;
    token::SpringSpec spec() const { return m_spec; }
    void              setSpec(token::SpringSpec);
    qreal             visibilityThreshold() const { return m_threshold; }
    void              setVisibilityThreshold(qreal);
    bool              isEnabled() const { return m_enabled; }
    void              setEnabled(bool);
    bool              isRunning() const { return m_running; }
    Q_INVOKABLE void  complete();
    Q_INVOKABLE void  stop();
    Q_SIGNAL void     specChanged();
    Q_SIGNAL void     visibilityThresholdChanged();
    Q_SIGNAL void     enabledChanged();
    Q_SIGNAL void     runningChanged();
    Q_SIGNAL void     targetChanged();
    Q_SIGNAL void     valueChanged();
    Q_SIGNAL void     finished();

protected:
    explicit SpringMotionBase(int dimensions, QObject* parent);
    void          setTargets(const QList<qreal>&, bool snap = false, bool preserveVelocity = true);
    qreal         target(int index) const;
    qreal         value(int index) const;
    qreal         velocity(int index) const;
    int           elapsed() const;
    virtual qreal displacementThreshold(int index) const;

private:
    friend class SpringClock;
    void restart();
    void tick(int);
    void settle(bool notifyFinished);
    struct Track {
        qreal              from = 0, target = 0, initialVelocity = 0;
        anim::SpringSample current;
        int                duration = 0;
    };
    QList<Track>                 m_tracks;
    std::unique_ptr<SpringClock> m_clock;
    token::SpringSpec            m_spec;
    qreal                        m_threshold = .01;
    bool                         m_enabled = true, m_complete = true, m_running = false;
    quint64                      m_revision = 0;
};

/** Scalar target tracking; velocity is measured in units per second. @ingroup impl */
class QML_MATERIAL_API SpringMotion : public SpringMotionBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal targetValue READ targetValue WRITE setTargetValue NOTIFY targetChanged FINAL)
    Q_PROPERTY(qreal value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal velocity READ velocity NOTIFY valueChanged FINAL)
public:
    explicit SpringMotion(QObject* parent = nullptr);
    qreal            targetValue() const { return target(0); }
    void             setTargetValue(qreal v) { setTargets({ v }); }
    qreal            value() const { return SpringMotionBase::value(0); }
    qreal            velocity() const { return SpringMotionBase::velocity(0); }
    Q_INVOKABLE void snapTo(qreal v) { setTargets({ v }, true); }
};

/** Oklab color target tracking with independent alpha. @ingroup impl */
class QML_MATERIAL_API ColorSpringMotion : public SpringMotionBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor targetColor READ targetColor WRITE setTargetColor NOTIFY targetChanged FINAL)
    Q_PROPERTY(QColor color READ color NOTIFY valueChanged FINAL)
public:
    explicit ColorSpringMotion(QObject* parent = nullptr);
    QColor targetColor() const;
    void   setTargetColor(const QColor&);
    QColor color() const;
};

/** Corner radii sharing a normalized spring response. @ingroup impl */
class QML_MATERIAL_API CornerSpringMotion : public SpringMotionBase {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qml_material::CornersGroup targetCorners READ targetCorners WRITE setTargetCorners
                   NOTIFY targetChanged FINAL)
    Q_PROPERTY(qml_material::CornersGroup corners READ corners NOTIFY valueChanged FINAL)
public:
    explicit CornerSpringMotion(QObject* parent = nullptr);
    CornersGroup targetCorners() const;
    void         setTargetCorners(const CornersGroup&);
    CornersGroup corners() const;

private:
    qreal        displacementThreshold(int index) const override;
    CornersGroup m_start;
};

/** Press feedback with a minimum visible expansion before release. @ingroup impl */
class QML_MATERIAL_API PressMotion : public SpringMotion {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged FINAL)
public:
    explicit PressMotion(QObject* parent = nullptr);
    bool          isPressed() const { return m_pressed; }
    void          setPressed(bool);
    bool          isActive() const { return m_active; }
    void          setActive(bool);
    Q_SIGNAL void pressedChanged();
    Q_SIGNAL void activeChanged();

private:
    void releaseIfReady();
    bool m_pressed = false, m_active = true, m_releasing = false;
};
} // namespace qml_material
