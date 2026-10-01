#pragma once

#include "qml_material/control/progress_bar.hpp"
#include "qml_material/indicator/progress_indicator_geometry.hpp"

namespace qml_material
{
/** @ingroup control */
class QML_MATERIAL_API ProgressIndicator : public ProgressBar {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(
        bool running READ running WRITE setRunning RESET resetRunning NOTIFY runningChanged FINAL)
    Q_PROPERTY(bool wavy READ wavy WRITE setWavy NOTIFY wavyChanged FINAL)
    Q_PROPERTY(bool animationsEnabled READ animationsEnabled WRITE setAnimationsEnabled NOTIFY
                   animationsEnabledChanged FINAL)
    Q_PROPERTY(bool animating READ animating NOTIFY frameChanged FINAL)
    Q_PROPERTY(
        qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged FINAL)
    Q_PROPERTY(qreal waveLength READ waveLength WRITE setWaveLength RESET resetWaveLength NOTIFY
                   waveLengthChanged FINAL)
    Q_PROPERTY(qreal waveAmplitude READ waveAmplitude WRITE setWaveAmplitude NOTIFY
                   waveAmplitudeChanged FINAL)
    Q_PROPERTY(int waveCycleDuration READ waveCycleDuration WRITE setWaveCycleDuration RESET
                   resetWaveCycleDuration NOTIFY waveCycleDurationChanged FINAL)
    Q_PROPERTY(
        qreal gapSize READ gapSize WRITE setGapSize RESET resetGapSize NOTIFY gapSizeChanged FINAL)
    Q_PROPERTY(qreal gapAngle READ gapAngle WRITE setGapAngle RESET resetGapAngle NOTIFY
                   gapAngleChanged FINAL)
    Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle NOTIFY startAngleChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor NOTIFY trackColorChanged FINAL)
    Q_PROPERTY(
        QColor inactiveColor READ trackColor WRITE setTrackColor NOTIFY trackColorChanged FINAL)
    Q_PROPERTY(QColor stopIndicatorColor READ stopIndicatorColor WRITE setStopIndicatorColor NOTIFY
                   stopIndicatorColorChanged FINAL)
    Q_PROPERTY(int type READ type WRITE setType RESET resetType NOTIFY typeChanged FINAL)
    Q_PROPERTY(AnimStateType animationState READ animationState WRITE setAnimationState NOTIFY
                   frameChanged FINAL)
    Q_PROPERTY(CompletionBehavior completionBehavior READ completionBehavior WRITE
                   setCompletionBehavior NOTIFY completionBehaviorChanged FINAL)
    Q_PROPERTY(qreal displayedPosition READ displayedPosition NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal phase READ phase NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal amplitudeFraction READ amplitudeFraction NOTIFY frameChanged FINAL)
    Q_PROPERTY(qreal progress READ progress NOTIFY frameChanged FINAL)
    Q_PROPERTY(QSizeF preferredSize READ preferredSize NOTIFY parametersChanged FINAL)
public:
    enum AnimStateType
    {
        Running    = 0,
        Completing = 1,
        Stopped    = 2
    };
    Q_ENUM(AnimStateType)
    enum CompletionBehavior
    {
        Keep  = 0,
        Drain = 1
    };
    Q_ENUM(CompletionBehavior)
    ~ProgressIndicator() override;
    bool                   running() const;
    bool                   wavy() const;
    bool                   animationsEnabled() const;
    bool                   animating() const;
    qreal                  strokeWidth() const;
    qreal                  waveLength() const;
    qreal                  waveAmplitude() const;
    int                    waveCycleDuration() const;
    qreal                  gapSize() const;
    qreal                  gapAngle() const;
    qreal                  startAngle() const;
    QColor                 color() const;
    QColor                 trackColor() const;
    QColor                 stopIndicatorColor() const;
    int                    type() const;
    AnimStateType          animationState() const;
    CompletionBehavior     completionBehavior() const;
    qreal                  displayedPosition() const;
    qreal                  phase() const;
    qreal                  amplitudeFraction() const;
    qreal                  progress() const;
    QSizeF                 preferredSize() const;
    ProgressIndicatorState renderState() const;
    void                   setRunning(bool value);
    void                   resetRunning();
    void                   setWavy(bool value);
    void                   setAnimationsEnabled(bool value);
    void                   setStrokeWidth(qreal value);
    void                   setWaveLength(qreal value);
    void                   resetWaveLength();
    void                   setWaveAmplitude(qreal value);
    void                   setWaveCycleDuration(int value);
    void                   resetWaveCycleDuration();
    void                   setGapSize(qreal value);
    void                   resetGapSize();
    void                   setGapAngle(qreal value);
    void                   resetGapAngle();
    void                   setStartAngle(qreal value);
    void                   setColor(QColor value);
    void                   setTrackColor(QColor value);
    void                   setStopIndicatorColor(QColor value);
    void                   setType(int value);
    void                   resetType();
    void                   setAnimationState(AnimStateType value);
    void                   setCompletionBehavior(CompletionBehavior value);
    Q_SIGNAL void          parametersChanged();
    Q_SIGNAL void          runningChanged();
    Q_SIGNAL void          wavyChanged();
    Q_SIGNAL void          animationsEnabledChanged();
    Q_SIGNAL void          strokeWidthChanged();
    Q_SIGNAL void          waveLengthChanged();
    Q_SIGNAL void          waveAmplitudeChanged();
    Q_SIGNAL void          waveCycleDurationChanged();
    Q_SIGNAL void          gapSizeChanged();
    Q_SIGNAL void          gapAngleChanged();
    Q_SIGNAL void          startAngleChanged();
    Q_SIGNAL void          colorChanged();
    Q_SIGNAL void          trackColorChanged();
    Q_SIGNAL void          stopIndicatorColorChanged();
    Q_SIGNAL void          typeChanged();
    Q_SIGNAL void          completionBehaviorChanged();
    Q_SIGNAL void          frameChanged();

protected:
    explicit ProgressIndicator(bool circular, QQuickItem* parent = nullptr);
    void componentComplete() override;
    void updatePolish() override;
    // Deterministic elapsed sampling, shared by the window clock and native tests.
    void advanceAnimations(qreal elapsedMs);

private:
    struct Private;
    std::unique_ptr<Private> d;
    void                     synchronize(bool immediate = false);
    void                     notifyParameters();
    void                     publish();
    void                     bindWindow(QQuickWindow* window);
    void                     observeViewport();
    void                     refreshViewport();
    void                     schedule();
    bool                     eligible() const;
    bool                     needsAnimation() const;
};

/** @ingroup control */
class QML_MATERIAL_API LinearIndicator : public ProgressIndicator {
    Q_OBJECT
    QML_NAMED_ELEMENT(LinearIndicatorBase)
public:
    enum AnimType
    {
        Disjoint   = 0,
        Contiguous = 1
    };
    Q_ENUM(AnimType)
    explicit LinearIndicator(QQuickItem* parent = nullptr): ProgressIndicator(false, parent) {}
};

/** @ingroup control */
class QML_MATERIAL_API CircularIndicator : public ProgressIndicator {
    Q_OBJECT
    QML_NAMED_ELEMENT(CircularIndicatorBase)
public:
    enum AnimType
    {
        Advance = 0,
        Reteat  = 1,
        Retreat = 1
    };
    Q_ENUM(AnimType)
    explicit CircularIndicator(QQuickItem* parent = nullptr): ProgressIndicator(true, parent) {}
};
} // namespace qml_material
