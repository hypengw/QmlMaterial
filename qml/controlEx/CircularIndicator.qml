import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

T.BusyIndicator {
    id: control

    property real strokeWidth: 4
    readonly property alias progress: m_updator.progress
    running: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    MD.CircularIndicatorUpdator {
        id: m_updator
    }

    NumberAnimation {
        running: true
        paused: !control.running
        loops: Animation.Infinite
        target: m_updator
        property: 'progress'
        from: 0
        to: 1
        duration: m_updator.duration
    }

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32
        MD.CircleIndicatorShape {
            id: m_shape
            anchors.fill: parent
            strokeWidth: control.strokeWidth
            rotation: m_updator.rotation
            startAngle: m_updator.startFraction * 360
            endAngle: m_updator.endFraction * 360
        }
    }
}
