pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

/**
 * @brief Material Design 3 circular progress indicator
 * @ingroup component
 *
 * Unified component:
 *  - `indeterminate: false` (default) → determinate, follows `value`/`position`
 *  - `indeterminate: true` → indeterminate animation driven by
 *    `CircularIndicatorUpdator` (Advance or Retreat via `type`)
 *  - `wavy: false` (default) renders flat arcs via CircleIndicatorShape
 *  - `wavy: true` renders the active arc as a wavy stroked path via
 *    CircleIndicatorWaveShape (M3 expressive)
 *
 * `running` toggles the indeterminate animation with a hide-fade. Determinate
 * is always visible regardless.
 */
T.ProgressBar {
    id: control
    enum AnimStateType {
        Running,
        Completing,
        Stopped
    }
    enum AnimType {
        Advance = 0,
        Reteat
    }

    property int animationState: CircularIndicator.Stopped
    property real strokeWidth: 4
    property color color: MD.MProp.color.primary
    property color inactiveColor: "transparent"
    property alias type: m_updator.indeterminateAnimationType
    readonly property alias progress: m_updator.progress

    property bool running: control.indeterminate

    property bool wavy: false
    // M3 spec: amplitude 1.6dp, wavelength 15dp.
    property real waveLength: 15
    property real waveAmplitude: 1.6
    property int waveCycleDuration: 1200

    // Sweep starts from this angle in determinate mode. -90 = top, matches M3.
    property real startAngle: -90
    // Angular gap between active and inactive arcs (degrees) in determinate mode.
    property real gapAngle: 8

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: strokeWidth
    clip: false

    onRunningChanged: __syncAnimState()
    onIndeterminateChanged: __syncAnimState()
    function __syncAnimState() {
        const shouldRun = control.running && control.indeterminate;
        if (shouldRun) {
            m_updator.completeEndProgress = 0;
            animationState = CircularIndicator.Running;
        } else if (animationState === CircularIndicator.Running) {
            animationState = CircularIndicator.Completing;
        }
    }

    MD.CircularIndicatorUpdator {
        id: m_updator
        // Wavy variant defaults to Retreat (M3 expressive); flat keeps Advance.
        // User can override via `type:` (alias breaks this binding).
        indeterminateAnimationType: control.wavy
            ? MD.CircularIndicatorUpdator.Reteat
            : MD.CircularIndicatorUpdator.Advance
    }

    // Wall-clock driven progress — see comment in original CircularIndicator.
    FrameAnimation {
        running: control.animationState !== CircularIndicator.Stopped
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
        running: control.animationState === CircularIndicator.Completing
        property real _startMs: 0
        onRunningChanged: { if (running) _startMs = Date.now(); }
        onTriggered: {
            const d = m_updator.completeEndDuration;
            const t = d > 0 ? Math.min(1, (Date.now() - _startMs) / d) : 1;
            m_updator.completeEndProgress = t;
            if (t >= 1 && control.animationState === CircularIndicator.Completing)
                control.animationState = CircularIndicator.Stopped;
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
    // spec (active sweep shrinks back to 0). The only animation in determinate
    // mode.
    readonly property bool __complete: ! control.indeterminate
                                        && control.position >= 1

    // 0 = full active, 1 = fully drained.
    property real __completionFraction: control.__complete ? 1 : 0
    Behavior on __completionFraction {
        NumberAnimation {
            duration: MD.Token.duration.long4
            easing.type: Easing.OutCubic
        }
    }

    // Determinate sweep: subtract two gaps so both ends have breathing room
    // against the inactive backdrop. Both active and inactive scale by
    // (1 - completionFraction) so the whole ring drains together on complete,
    // anchored at startAngle.
    readonly property real __detActiveSweep:
        Math.max(0, control.__displayedPosition * 360 - control.gapAngle * 2)
            * (1 - control.__completionFraction)
    readonly property real __detInactiveSweep:
        Math.max(0, 360 * (1 - control.__displayedPosition))
            * (1 - control.__completionFraction)

    readonly property real __activeStart:
        control.indeterminate ? m_updator.startFraction * 360 : control.startAngle
    readonly property real __activeEnd:
        control.indeterminate ? m_updator.endFraction * 360 : (control.startAngle + __detActiveSweep)

    readonly property real __inactiveStart:
        control.indeterminate ? 0 : (control.startAngle + __detActiveSweep + control.gapAngle)
    readonly property real __inactiveSweep:
        control.indeterminate ? 360 : __detInactiveSweep

    // Indeterminate uses opacity-based hide-fade. Determinate stays visible —
    // the drain animation handles the disappearance.
    readonly property bool __visible:
        ! control.indeterminate || control.animationState !== CircularIndicator.Stopped

    Component {
        id: c_flat_shape
        MD.CircleIndicatorShape {
            rotation: control.indeterminate ? m_updator.rotation : 0
            strokeColor: control.color
            inactiveColor: control.inactiveColor
            strokeWidth: control.strokeWidth
            startAngle: control.__activeStart
            endAngle: control.__activeEnd
            inactiveStartAngle: control.__inactiveStart
            inactiveSweepAngle: control.__inactiveSweep
        }
    }

    Component {
        id: c_wave_shape
        MD.CircleIndicatorWaveShape {
            rotation: control.indeterminate ? m_updator.rotation : 0
            strokeColor: control.color
            inactiveColor: control.inactiveColor
            strokeWidth: control.strokeWidth
            startAngle: control.__activeStart
            endAngle: control.__activeEnd
            inactiveStartAngle: control.__inactiveStart
            inactiveSweepAngle: control.__inactiveSweep
            waveLength: control.waveLength
            waveAmplitude: control.waveAmplitude * control.__ampFraction
            phase: control.__phase
        }
    }

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32

        opacity: control.__visible ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: MD.Token.duration.short2 }
        }

        MD.Loader {
            anchors.fill: parent
            sourceComponent: control.wavy ? c_wave_shape : c_flat_shape
        }
    }

    background: Item {}
}
