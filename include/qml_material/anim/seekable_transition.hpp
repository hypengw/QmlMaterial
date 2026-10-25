#pragma once
#include "qml_material/anim/animatable.hpp"
#include <QQmlListProperty>
#include <QHash>
namespace qml_material
{
class SeekableTransition;
class QML_MATERIAL_API TransitionTrack : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QVariantMap values READ values WRITE setValues NOTIFY definitionChanged FINAL)
    Q_PROPERTY(AnimationSpec spec READ spec WRITE setSpec NOTIFY definitionChanged FINAL)
public:
    QVariantMap     values() const { return m_values; }
    void            setValues(QVariantMap);
    AnimationSpec   spec() const { return m_spec; }
    void            setSpec(AnimationSpec);
    anim::ValueKind kind() const { return m_kind; }
    QVariant        valueVariant() const { return anim::decode(m_sample.value, m_kind); }
    QVariant      velocityVariant() const { return anim::velocityValue(m_sample.velocity, m_kind); }
    Q_SIGNAL void valueChanged();
    Q_SIGNAL void definitionChanged();

protected:
    TransitionTrack(anim::ValueKind, QObject*);

private:
    friend class SeekableTransition;
    anim::ValueKind              m_kind;
    QVariantMap                  m_values;
    AnimationSpec                m_spec;
    anim::Sample                 m_sample;
    QPointer<SeekableTransition> m_owner;
};
class QML_MATERIAL_API RealTrack : public TransitionTrack {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal velocity READ velocity NOTIFY valueChanged FINAL)
public:
    explicit RealTrack(QObject* p = nullptr): TransitionTrack(anim::ValueKind::Real, p) {}
    qreal value() const { return valueVariant().value<qreal>(); }
    qreal velocity() const { return velocityVariant().value<qreal>(); }
};
class QML_MATERIAL_API PointTrack : public TransitionTrack {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QPointF value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(QPointF velocity READ velocity NOTIFY valueChanged FINAL)
public:
    explicit PointTrack(QObject* p = nullptr): TransitionTrack(anim::ValueKind::Point, p) {}
    QPointF value() const { return valueVariant().value<QPointF>(); }
    QPointF velocity() const { return velocityVariant().value<QPointF>(); }
};
class QML_MATERIAL_API SizeTrack : public TransitionTrack {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QSizeF value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(QSizeF velocity READ velocity NOTIFY valueChanged FINAL)
public:
    explicit SizeTrack(QObject* p = nullptr): TransitionTrack(anim::ValueKind::Size, p) {}
    QSizeF value() const { return valueVariant().value<QSizeF>(); }
    QSizeF velocity() const { return velocityVariant().value<QSizeF>(); }
};
class QML_MATERIAL_API ColorTrack : public TransitionTrack {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QColor value READ value NOTIFY valueChanged FINAL)
    Q_PROPERTY(QVector4D velocity READ velocity NOTIFY valueChanged FINAL)
public:
    explicit ColorTrack(QObject* p = nullptr): TransitionTrack(anim::ValueKind::Color, p) {}
    QColor    value() const { return valueVariant().value<QColor>(); }
    QVector4D velocity() const { return velocityVariant().value<QVector4D>(); }
};
class QML_MATERIAL_API SeekableTransition : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "tracks")
    Q_PROPERTY(QQmlListProperty<TransitionTrack> tracks READ tracks FINAL)
    Q_PROPERTY(QString initialState READ initialState WRITE setInitialState NOTIFY
                   initialStateChanged FINAL)
    Q_PROPERTY(QString currentState READ currentState NOTIFY changed FINAL)
    Q_PROPERTY(QString targetState READ targetState NOTIFY changed FINAL)
    Q_PROPERTY(qreal fraction READ fraction NOTIFY changed FINAL)
    Q_PROPERTY(qreal playTime READ playTime NOTIFY changed FINAL)
    Q_PROPERTY(int state READ state NOTIFY changed FINAL)
    Q_PROPERTY(int duration READ duration NOTIFY changed FINAL)
    Q_PROPERTY(bool active READ active NOTIFY changed FINAL)
    Q_PROPERTY(bool running READ running NOTIFY changed FINAL)
    Q_PROPERTY(bool paused READ paused NOTIFY changed FINAL)
    Q_PROPERTY(bool manualClock READ manualClock WRITE setManualClock NOTIFY clockChanged FINAL)
    Q_PROPERTY(MotionSettings* motion READ settings WRITE setSettings NOTIFY settingsChanged FINAL)
public:
    enum Endpoint
    {
        Origin,
        Destination
    };
    Q_ENUM(Endpoint)
    explicit SeekableTransition(QObject* parent = nullptr);
    ~SeekableTransition() override;
    void                              classBegin() override { m_initializing = true; }
    void                              componentComplete() override;
    QQmlListProperty<TransitionTrack> tracks();
    bool                              addTrack(TransitionTrack*);
    QString                           initialState() const { return m_initial; }
    void                              setInitialState(QString);
    QString                           currentState() const { return m_current; }
    QString                           targetState() const { return m_target; }
    qreal                             fraction() const { return m_fraction; }
    qreal                             playTime() const { return m_fraction * m_duration; }
    int                               state() const { return m_driver.state(); }
    int                               duration() const { return m_duration; }
    bool                              active() const { return m_active; }
    bool running() const { return m_driver.state() == MotionRunner::Running; }
    bool paused() const { return m_driver.state() == MotionRunner::Paused; }
    bool manualClock() const { return m_driver.manualClock(); }
    void setManualClock(bool v) {
        m_driver.setManualClock(v);
        emit clockChanged();
    }
    MotionSettings*     settings() const { return m_settings; }
    void                setSettings(MotionSettings*);
    Q_INVOKABLE QString animateTo(QString state);
    Q_INVOKABLE bool    snapTo(QString state);
    Q_INVOKABLE QString beginSeek(QString state);
    Q_INVOKABLE bool    seek(QString id, qreal fraction);
    Q_INVOKABLE QString settle(QString id, Endpoint endpoint, qreal velocity, AnimationSpec spec);
    Q_INVOKABLE void    stop();
    Q_INVOKABLE void    pause() { m_driver.pause(); }
    Q_INVOKABLE void    resume() { m_driver.resume(); }
    Q_INVOKABLE void    complete() { m_driver.complete(); }
    Q_INVOKABLE void    advance(qreal milliseconds) { m_driver.advance(milliseconds); }
    Q_SIGNAL void       changed();
    Q_SIGNAL void       initialStateChanged();
    Q_SIGNAL void       clockChanged();
    Q_SIGNAL void       settingsChanged();
    Q_SIGNAL void       ended(QString id, int reason, QString state);

private:
    struct Track {
        QPointer<TransitionTrack> track;
        anim::PreparedMotion      motion;
    };
    bool                             prepare(const QString&);
    void                             sample();
    void                             initialize();
    void                             refreshPolicy();
    QList<QPointer<TransitionTrack>> m_tracks;
    QList<Track>                     m_prepared;
    MotionRunner                     m_driver { 1, this };
    QPointer<MotionSettings>         m_settings;
    QMetaObject::Connection          m_policy, m_policyDestroyed;
    QString m_initial = "from", m_current = "from", m_origin, m_target = "from", m_run;
    QHash<QString, QString> m_runs;
    qreal                   m_fraction = 0;
    int                     m_duration = 0;
    bool                    m_active = false, m_configuring = false, m_initializing = false;
};
} // namespace qml_material
