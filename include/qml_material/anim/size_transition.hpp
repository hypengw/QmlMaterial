#pragma once
#include "qml_material/anim/motion_runner.hpp"
#include "qml_material/style/motion_settings.hpp"
#include <QQmlParserStatus>
#include <QSizeF>
namespace qml_material
{
class QML_MATERIAL_API SizeTransition : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QSizeF targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged FINAL)
    Q_PROPERTY(
        QSizeF maximumSize READ maximumSize WRITE setMaximumSize NOTIFY maximumSizeChanged FINAL)
    Q_PROPERTY(QSizeF size READ size NOTIFY sizeChanged FINAL)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(AnimationSpec spec READ spec WRITE setSpec RESET resetSpec NOTIFY specChanged FINAL)
    Q_PROPERTY(MotionSettings* motion READ settings WRITE setSettings NOTIFY settingsChanged FINAL)
public:
    explicit SizeTransition(QObject* parent = nullptr);
    QSizeF           targetSize() const { return m_target; }
    QSizeF           maximumSize() const { return m_maximum; }
    QSizeF           size() const { return m_size; }
    bool             active() const { return m_active; }
    bool             running() const { return m_running; }
    bool             enabled() const { return m_enabled; }
    AnimationSpec    spec() const;
    void             setTargetSize(QSizeF);
    void             setMaximumSize(QSizeF);
    void             setEnabled(bool);
    void             setSpec(AnimationSpec);
    void             resetSpec();
    MotionSettings*  settings() const { return m_settings; }
    void             setSettings(MotionSettings*);
    MotionRunner&    runner() { return m_runner; }
    Q_INVOKABLE void complete();
    Q_INVOKABLE void pause() { m_runner.pause(); }
    Q_INVOKABLE void resume() { m_runner.resume(); }
    QString          beginSeek(QSizeF destination);
    bool             seek(QString id, qreal progress);
    void             endSeek(QString id, bool commit);
    void             classBegin() override { m_complete = false; }
    void             componentComplete() override;
    Q_SIGNAL void    targetSizeChanged();
    Q_SIGNAL void    maximumSizeChanged();
    Q_SIGNAL void    sizeChanged();
    Q_SIGNAL void    activeChanged();
    Q_SIGNAL void    runningChanged();
    Q_SIGNAL void    enabledChanged();
    Q_SIGNAL void    specChanged();
    Q_SIGNAL void    settingsChanged();

private:
    QSizeF                       bounded(QSizeF) const;
    void                         retarget(bool force = false);
    void                         synchronize();
    void                         refreshPolicy();
    MotionRunner                 m_runner { 2, this };
    QPointer<MotionSettings>     m_settings;
    QMetaObject::Connection      m_policy, m_policyDestroyed;
    std::optional<AnimationSpec> m_spec;
    QSizeF               m_target { 0, 0 }, m_maximum { -1, -1 }, m_size { 0, 0 }, m_seekOrigin;
    anim::PreparedMotion m_seekMotion;
    QString              m_seek;
    bool    m_enabled = true, m_complete = true, m_initialized = false, m_hasTarget = false;
    bool    m_running = false, m_active = false, m_configuring = false;
    quint64 m_revision = 0;
};
} // namespace qml_material
