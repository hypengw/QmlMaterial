import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T

import Qcm.Material as MD

T.BusyIndicator {
    id: control
    readonly property real arcDuration: 1333

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32
        MD.CircleIndicatorShape {
            id: m_shape
            anchors.fill: parent

            function reset() {
                const p = m_shape;
                p.startAngle = (p.startAngle + p.sweepAngle) % 360;
                p.sweepAngle = -p.sweepAngle;
            }

            ParallelAnimation {
                running: control.running
                loops: Animation.Infinite
                RotationAnimator {
                    target: control
                    from: 0
                    to: 360
                    duration: (4 * control.arcDuration) / 360 * 306
                }
                RotationAnimator {
                    target: m_shape
                    from: 0
                    to: 1080
                    duration: 4 * control.arcDuration
                }
                SequentialAnimation {
                    ExpandArc {}
                    ContractArc {}
                    ExpandArc {}
                    ContractArc {}
                }
            }
        }
    }
    component ExpandArc: SequentialAnimation {
        ScriptAction {
            script: m_shape.reset()
        }
        NumberAnimation {
            target: m_shape
            property: 'sweepAngle'
            to: 270
            duration: 0.5 * control.arcDuration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: [0.4, 0, 0.2, 1, 1, 1]
        }
        NumberAnimation {
            duration: 0.5 * control.arcDuration
        }
    }
    component ContractArc: SequentialAnimation {
        ScriptAction {
            script: m_shape.reset()
        }
        NumberAnimation {
            target: m_shape
            property: 'sweepAngle'
            to: -10
            duration: 0.5 * control.arcDuration
            easing.type: Easing.BezierSpline
            easing.bezierCurve: [0.4, 0, 0.2, 1, 1, 1]
        }
        NumberAnimation {
            duration: 0.5 * control.arcDuration
        }
    }
    ScriptAction {
        script: m_shape.reset()
    }
    NumberAnimation {
        target: m_shape
        property: 'sweepAngle'
        to: -10
        duration: 0.5 * control.arcDuration
    }

    background: Item {
        implicitHeight: 64
        implicitWidth: 64
    }
}
