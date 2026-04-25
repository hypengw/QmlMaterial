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
        Disjoint = 0,
        Contiguous
    }

    property int animationState: LinearIndicator.Stopped
    property int strokeWidth: 4
    property alias type: m_updator.indeterminateAnimationType
    running: false
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    onRunningChanged: {
        if (running) {
            m_updator.completeEndProgress = 0;
            animationState = LinearIndicator.Running;
        } else {
            if (animationState == LinearIndicator.Running)
                animationState = LinearIndicator.Completing;
        }
    }

    MD.LinearIndicatorUpdator {
        id: m_updator
        colors: {
            const m = control.MD.MProp.color;
            if (indeterminateAnimationType == MD.LinearIndicatorUpdator.DisJoint) {
                return [m.primary];
            } else {
                return [m.primary, m.primary, m.primary];
            }
        }
    }

    // Wall-clock driven progress — see CircularIndicator.qml for rationale.
    FrameAnimation {
        running: control.animationState != LinearIndicator.Stopped
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
        running: control.animationState == LinearIndicator.Completing
        property real _startMs: 0
        property real _from: 0
        onRunningChanged: { if (running) { _startMs = Date.now(); _from = m_updator.progress; } }
        onTriggered: {
            const d = m_updator.completeEndDuration;
            const t = d > 0 ? Math.min(1, (Date.now() - _startMs) / d) : 1;
            m_updator.progress = _from + (1 - _from) * t;
            if (t >= 1 && control.animationState == LinearIndicator.Completing)
                control.animationState = LinearIndicator.Stopped;
        }
    }

    contentItem: Item {
        implicitHeight: m_shape.strokeWidth
        implicitWidth: 100
        MD.LinearIndicatorShape {
            id: m_shape
            anchors.fill: parent
            indicators: m_updator.activeIndicators
            strokeWidth: control.animationState != LinearIndicator.Stopped ? control.strokeWidth : 0

            Behavior on strokeWidth {
                NumberAnimation {
                    duration: MD.Token.duration.short2
                }
            }
        }
    }

    background: Item {}
}
