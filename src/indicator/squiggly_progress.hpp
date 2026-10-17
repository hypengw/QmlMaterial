#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <private/qquickshape_p.h>
#include "qml_material/core/item_activity.hpp"
#include "qml_material/indicator/squiggly_progress_geometry.hpp"

namespace qml_material
{
/** @ingroup control */
class SquigglyProgress : public QQuickShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(bool animate READ animate WRITE setAnimate NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY
                   appearanceChanged FINAL)
    Q_PROPERTY(bool transitionEnabled READ transitionEnabled WRITE setTransitionEnabled NOTIFY
                   appearanceChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal waveLength READ waveLength WRITE setWaveLength NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude NOTIFY
                   appearanceChanged FINAL)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(
        qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY appearanceChanged FINAL)
    Q_PROPERTY(qreal phase READ phase NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal amplitudeFraction READ amplitudeFraction NOTIFY frameChanged FINAL)
    Q_PROPERTY(bool animating READ animating NOTIFY frameChanged FINAL)
public:
    explicit SquigglyProgress(QQuickItem* parent = nullptr);
    ~SquigglyProgress() override;
    qreal         progress() const { return m_state.progress; }
    bool          animate() const { return m_animate; }
    bool          animationsEnabled() const { return m_animations; }
    bool          transitionEnabled() const { return m_state.transitionEnabled; }
    QColor        color() const { return m_color; }
    qreal         waveLength() const { return m_state.waveLength; }
    qreal         waveAmplitude() const { return m_state.waveAmplitude; }
    qreal         waveSpeed() const { return m_speed; }
    qreal         strokeWidth() const { return m_stroke; }
    qreal         phase() const { return m_state.phase; }
    qreal         amplitudeFraction() const { return m_state.amplitudeFraction; }
    bool          animating() const { return m_active; }
    void          setProgress(qreal);
    void          setAnimate(bool);
    void          setAnimationsEnabled(bool);
    void          setTransitionEnabled(bool);
    void          setColor(QColor);
    void          setWaveLength(qreal);
    void          setWaveAmplitude(qreal);
    void          setWaveSpeed(qreal);
    void          setStrokeWidth(qreal);
    Q_SIGNAL void appearanceChanged();
    Q_SIGNAL void frameChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF&, const QRectF&) override;
    void updatePolish() override;
    void advanceAnimations(qreal elapsedMs);

private:
    void                          invalidate();
    void                          schedule();
    void                          bindWindow(QQuickWindow*);
    void                          updatePaths();
    SquigglyProgressState         m_state;
    QColor                        m_color = Qt::black;
    qreal                         m_speed = 8, m_stroke = 2, m_elapsed = 0, m_from = 0;
    bool                          m_animate = false, m_animations = true, m_active = false;
    bool                          m_complete = false, m_dirty = true, m_heightAnimating = false;
    QElapsedTimer                 m_clock;
    QPointer<QQuickWindow>        m_window;
    QMetaObject::Connection       m_frameConnection;
    std::unique_ptr<ItemActivity> m_activity;
    QList<QQuickShapePath*>       m_paths;
};
} // namespace qml_material
