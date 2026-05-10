pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

/**
 * @brief Material Design 3 linear progress indicator
 * @ingroup component
 *
 * Unified component:
 *  - `indeterminate: false` (default) → determinate, follows `value`/`position`
 *  - `indeterminate: true` → indeterminate animation driven by
 *    `LinearIndicatorUpdator` (Disjoint or Contiguous via `type`)
 *  - `wavy: false` (default) renders flat segments via LinearIndicatorShape
 *  - `wavy: true` renders the active segment(s) as wavy paths via
 *    LinearIndicatorWaveShape (M3 expressive)
 */
T.ProgressBar {
    id: control
    enum AnimStateType {
        Running,
        Completing,
        Stopped
    }
    enum AnimType {
        Disjoint = 0,
        Contiguous
    }

    property int animationState: LinearIndicator.Stopped
    property int strokeWidth: 4
    property alias type: m_updator.indeterminateAnimationType
    property color color: control.MD.MProp.color.primary
    property color trackColor: control.MD.MProp.color.secondary_container
    property color stopIndicatorColor: control.color

    // Toggles the indeterminate animation visibility with a hide-fade. Has no
    // effect in determinate mode (`indeterminate: false`), where the bar is
    // always shown. Defaults to `indeterminate` so a bare
    // `MD.LinearIndicator { indeterminate: true }` auto-animates.
    property bool running: control.indeterminate

    property bool wavy: false
    property real waveLength: 30
    property real waveAmplitude: 3
    property int waveCycleDuration: 1200

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    onRunningChanged: __syncAnimState()
    onIndeterminateChanged: __syncAnimState()
    function __syncAnimState() {
        const shouldRun = control.running && control.indeterminate;
        if (shouldRun) {
            m_updator.completeEndProgress = 0;
            animationState = LinearIndicator.Running;
        } else if (animationState === LinearIndicator.Running) {
            animationState = LinearIndicator.Completing;
        }
    }

    MD.LinearIndicatorUpdator {
        id: m_updator
        colors: {
            const c = control.color;
            if (indeterminateAnimationType === MD.LinearIndicatorUpdator.DisJoint) {
                return [c];
            } else {
                return [c, c, c];
            }
        }
    }

    // Wall-clock driven progress — see CircularIndicator.qml for rationale.
    FrameAnimation {
        running: control.animationState !== LinearIndicator.Stopped
        property real _startMs: 0
        onRunningChanged: { if (running) _startMs = Date.now(); }
        onTriggered: {
            const d = m_updator.duration;
            if (d > 0)
                m_updator.progress = ((Date.now() - _startMs) % d) / d;
        }
    }

    FrameAnimation {
        id: m_complete_end_anim
        running: control.animationState === LinearIndicator.Completing
        property real _startMs: 0
        property real _from: 0
        onRunningChanged: { if (running) { _startMs = Date.now(); _from = m_updator.progress; } }
        onTriggered: {
            const d = m_updator.completeEndDuration;
            const t = d > 0 ? Math.min(1, (Date.now() - _startMs) / d) : 1;
            m_updator.progress = _from + (1 - _from) * t;
            if (t >= 1 && control.animationState === LinearIndicator.Completing)
                control.animationState = LinearIndicator.Stopped;
        }
    }

    NumberAnimation on __phase {
        loops: Animation.Infinite
        from: 0
        to: 1
        duration: control.waveCycleDuration
        easing.type: Easing.Linear
        running: control.wavy && control.enabled
                 && (control.indeterminate
                     || (control.value < control.to && control.value > control.from))
    }
    property real __phase: 0

    property real __ampFraction: control.wavy && control.enabled ? 1.0 : 0.0
    Behavior on __ampFraction {
        NumberAnimation {
            duration: MD.Token.duration.medium2
            easing: MD.Token.easing.standard
        }
    }

    // Smoothed position — animates on every value change (including the jump
    // to 1 that triggers complete). Behavior duration is short so transitions
    // feel snappy, not draggy.
    property real __displayedPosition: control.position
    Behavior on __displayedPosition {
        NumberAnimation {
            duration: MD.Token.duration.short3
            easing: MD.Token.easing.standard
        }
    }

    // Determinate completes when position reaches the maximum — drains per M3
    // spec (active bar shrinks right-to-left), then the residual track height
    // collapses to 0.
    readonly property bool __complete: ! control.indeterminate
                                        && control.position >= 1

    // Composite phase: 0 = pre-complete, 1 = fully gone.
    // First 2/3 = drain active right-to-left; last 1/3 = shrink track height.
    property real __completionFraction: control.__complete ? 1 : 0
    Behavior on __completionFraction {
        NumberAnimation {
            duration: MD.Token.duration.long4 + MD.Token.duration.medium2
            easing.type: Easing.OutCubic
        }
    }
    readonly property real __drainAmount:
        Math.min(1, control.__completionFraction / 0.67)
    readonly property real __heightAmount:
        Math.max(0, (control.__completionFraction - 0.67) / 0.33)

    // Determinate synthesizes a single JS-object indicator. End drains on
    // complete; the inactive after-segment naturally vanishes when the height
    // collapses (see __heightAmount usage below).
    readonly property var __activeIndicators:
        control.indeterminate
            ? m_updator.activeIndicators
            : [{ startFraction: 0,
                 endFraction: control.__displayedPosition * (1 - control.__drainAmount),
                 color: control.color,
                 gapSize: 4 }]

    // Indeterminate uses opacity-based hide-fade. Determinate stays visible —
    // the drain animation handles the disappearance.
    readonly property bool __visible:
        ! control.indeterminate || control.animationState !== LinearIndicator.Stopped

    Component {
        id: c_flat_shape
        MD.LinearIndicatorShape {
            indicators: control.__activeIndicators
            trackColor: control.trackColor
            strokeWidth: control.strokeWidth * (1 - control.__heightAmount)
        }
    }

    Component {
        id: c_wave_shape
        MD.LinearIndicatorWaveShape {
            indicators: control.__activeIndicators
            trackColor: control.trackColor
            waveLength: control.waveLength
            waveAmplitude: control.waveAmplitude * control.__ampFraction
            phase: control.__phase
            strokeWidth: control.strokeWidth * (1 - control.__heightAmount)
        }
    }

    contentItem: Item {
        implicitHeight: control.strokeWidth
        implicitWidth: 100

        opacity: control.__visible ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: MD.Token.duration.short2 }
        }

        MD.Loader {
            anchors.fill: parent
            sourceComponent: control.wavy ? c_wave_shape : c_flat_shape
        }

        // M3 stop indicator dot (determinate only). Drains together with the
        // active bar on completion.
        MD.Rectangle {
            x: parent.width - 4
            y: parent.height / 2 - 2
            width: 4 * (1 - control.__heightAmount)
            height: 4 * (1 - control.__heightAmount)
            radius: 2
            color: control.stopIndicatorColor
            visible: ! control.indeterminate
            opacity: 1 - control.__drainAmount
        }
    }

    background: Item {}
}
