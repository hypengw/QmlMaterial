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

    property int animationState: LinearIndicator.Stopped
    running: false
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
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

    MD.LinearIndicatorUpdator {
        id: m_updator
        colors: {
            const m = control.MD.MProp.color;
            return [m.secondary_container, m.primary, m.secondary_container];
        }
    }

    NumberAnimation {
        running: control.animationState != LinearIndicator.Stopped
        loops: Animation.Infinite
        target: m_updator
        property: 'progress'
        from: 0
        to: 1
        duration: m_updator.duration
    }

    contentItem: Item {
        implicitHeight: 4
        implicitWidth: 100
        MD.LinearIndicatorShape {
            id: m_shape
            anchors.fill: parent
            indicators: m_updator.activeIndicators
        }
    }

    background: Item {}
}
