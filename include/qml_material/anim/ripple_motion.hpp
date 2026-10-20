#pragma once
#include <QAbstractAnimation>
#include <QQmlParserStatus>
#include <memory>
#include "qml_material/style/indication_policy.hpp"
#include "qml_material/core/item_activity.hpp"

namespace qml_material
{
class RippleClock;
/** Skia press presentation and persistent state-layer animation. @ingroup impl */
class QML_MATERIAL_API RippleMotion : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(InteractionSource* interactionSource READ interactionSource WRITE
                   setInteractionSource NOTIFY inputChanged FINAL)
    Q_PROPERTY(qml_material::IndicationValues indication READ indication WRITE setIndication RESET
                   resetIndication NOTIFY inputChanged FINAL)
    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY inputChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY inputChanged FINAL)
    Q_PROPERTY(qreal stateOpacity READ stateOpacity WRITE setStateOpacity NOTIFY inputChanged FINAL)
    Q_PROPERTY(qreal pressOpacity READ pressOpacity WRITE setPressOpacity NOTIFY inputChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY inputChanged FINAL)
    Q_PROPERTY(qreal opacity READ opacity NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal rippleOpacity READ rippleOpacity NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal progress READ progress NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal phase READ phase NOTIFY valueChanged FINAL)
    Q_PROPERTY(QPointF origin READ origin NOTIFY valueChanged FINAL)
    Q_PROPERTY(QColor ink READ ink NOTIFY valueChanged FINAL)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged FINAL)
public:
    explicit RippleMotion(QObject* parent = nullptr);
    ~RippleMotion() override;
    void               classBegin() override { m_complete = false; }
    void               componentComplete() override;
    InteractionSource* interactionSource() const { return m_source; }
    void               setInteractionSource(InteractionSource*);
    IndicationValues   indication() const;
    void               setIndication(const IndicationValues&);
    void               resetIndication();
    QQuickItem*        target() const { return m_target; }
    void               setTarget(QQuickItem*);
    bool               isEnabled() const { return m_enabled; }
    void               setEnabled(bool);
    qreal              stateOpacity() const { return m_stateOpacity; }
    void               setStateOpacity(qreal);
    qreal              pressOpacity() const { return m_pressOpacity; }
    void               setPressOpacity(qreal);
    QColor             color() const { return m_color; }
    void               setColor(const QColor&);
    qreal              opacity() const { return m_opacity; }
    qreal              rippleOpacity() const { return m_rippleOpacity; }
    qreal              progress() const { return m_progress; }
    qreal              phase() const { return m_phase; }
    QPointF            origin() const { return m_origin; }
    QColor             ink() const { return m_ink; }
    bool               running() const { return m_running; }
    Q_SIGNAL void      inputChanged();
    Q_SIGNAL void      valueChanged();
    Q_SIGNAL void      runningChanged();

private:
    friend class RippleClock;
    void                            updateTarget(bool snap = false);
    void                            startPress(quint64, const QPointF&);
    void                            endPress(quint64, Interaction::Phase);
    void                            reset();
    void                            tick(int);
    void                            schedule();
    bool                            active() const;
    int                             now() const;
    QPointer<InteractionSource>     m_source;
    std::optional<IndicationValues> m_indication;
    QPointer<QQuickItem>            m_target;
    QList<QMetaObject::Connection>  m_sourceConnections;
    std::unique_ptr<ItemActivity>   m_activity;
    std::unique_ptr<RippleClock>    m_clock;
    QMetaObject::Connection         m_targetDestroyed;
    bool                            m_hasTarget = false;
    bool                            m_enabled = true, m_complete = true, m_running = false;
    bool                            m_wave = false, m_releasing = false;
    quint64                         m_pressId = 0, m_revision = 0;
    int     m_kind = Interaction::None, m_stateStart = 0, m_stateDuration = 0;
    int     m_pressStart = 0, m_releaseStart = 0;
    qreal   m_stateOpacity = 0, m_pressOpacity = token::State {}.pressed.state_layer_opacity;
    qreal   m_opacity = 0, m_fromOpacity = 0, m_toOpacity = 0;
    qreal   m_progress = 0, m_phase = 0, m_rippleOpacity = 0, m_releaseFrom = 0, m_waveAlpha = 0;
    QPointF m_origin;
    QColor  m_color { Qt::transparent }, m_ink { Qt::transparent };
    IndicationPolicy m_defaultIndication;
};
} // namespace qml_material
