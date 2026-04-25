import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

T.BusyIndicator {
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
    property alias type: m_updator.indeterminateAnimationType
    readonly property alias progress: m_updator.progress

    running: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: strokeWidth
    clip: false

    onRunningChanged: {
        if (running) {
            m_updator.completeEndProgress = 0;
            animationState = CircularIndicator.Running;
        } else {
            if (animationState == CircularIndicator.Running)
                animationState = CircularIndicator.Completing;
        }
    }

    MD.CircularIndicatorUpdator {
        id: m_updator
    }

    // Drive progress from wall-clock Date.now() rather than NumberAnimation —
    // QQuickAnimationDriver advances animations per-frame on threaded render
    // loops, so on high-refresh-rate displays NumberAnimation ticks faster
    // than its declared duration. FrameAnimation gives vsync alignment while
    // the wall-clock formula keeps spin speed refresh-rate independent.
    FrameAnimation {
        running: control.animationState != CircularIndicator.Stopped
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
        running: control.animationState == CircularIndicator.Completing
        property real _startMs: 0
        onRunningChanged: { if (running) _startMs = Date.now(); }
        onTriggered: {
            const d = m_updator.completeEndDuration;
            const t = d > 0 ? Math.min(1, (Date.now() - _startMs) / d) : 1;
            m_updator.completeEndProgress = t;
            if (t >= 1 && control.animationState == CircularIndicator.Completing)
                control.animationState = CircularIndicator.Stopped;
        }
    }

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32
        MD.CircleIndicatorShape {
            id: m_shape
            anchors.fill: parent
            strokeColor: control.color
            rotation: m_updator.rotation
            startAngle: m_updator.startFraction * 360
            endAngle: m_updator.endFraction * 360

            strokeWidth: control.strokeWidth
        }
    }
}
