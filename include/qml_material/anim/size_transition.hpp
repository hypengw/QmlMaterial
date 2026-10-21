#pragma once

#include "qml_material/export.hpp"
#include <QQmlParserStatus>
#include <QSizeF>
#include <QVariantAnimation>
#include <qqmlregistration.h>

namespace qml_material
{
/** Tracks a bounded display size without changing the content's natural measurement. @ingroup impl
 */
class QML_MATERIAL_API SizeTransition : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QSizeF targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged FINAL)
    Q_PROPERTY(
        QSizeF maximumSize READ maximumSize WRITE setMaximumSize NOTIFY maximumSizeChanged FINAL)
    Q_PROPERTY(QSizeF size READ size NOTIFY sizeChanged FINAL)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged FINAL)

public:
    explicit SizeTransition(QObject* parent = nullptr);
    QSizeF           targetSize() const { return m_target; }
    QSizeF           maximumSize() const { return m_maximum; }
    QSizeF           size() const { return m_size; }
    bool             running() const { return m_running; }
    bool             enabled() const { return m_enabled; }
    int              duration() const { return m_duration; }
    QEasingCurve     easing() const { return m_easing; }
    void             setTargetSize(const QSizeF&);
    void             setMaximumSize(const QSizeF&);
    void             setEnabled(bool);
    void             setDuration(int);
    void             setEasing(const QEasingCurve&);
    Q_INVOKABLE void complete();
    void             classBegin() override;
    void             componentComplete() override;
    Q_SIGNAL void    targetSizeChanged();
    Q_SIGNAL void    maximumSizeChanged();
    Q_SIGNAL void    sizeChanged();
    Q_SIGNAL void    runningChanged();
    Q_SIGNAL void    enabledChanged();
    Q_SIGNAL void    durationChanged();
    Q_SIGNAL void    easingChanged();

private:
    QSizeF            bounded(const QSizeF&) const;
    void              retarget(bool restart = false);
    void              setSize(const QSizeF&);
    void              setRunning(bool);
    QVariantAnimation m_animation;
    int               m_duration;
    QEasingCurve      m_easing;
    QSizeF            m_target { 0, 0 };
    QSizeF            m_maximum { -1, -1 };
    QSizeF            m_size { 0, 0 };
    bool              m_enabled     = true;
    bool              m_running     = false;
    bool              m_complete    = true;
    bool              m_initialized = false;
    bool              m_hasTarget   = false;
    bool              m_configuring = false;
    quint64           m_revision    = 0;
};
} // namespace qml_material
