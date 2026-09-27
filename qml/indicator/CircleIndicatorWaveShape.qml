import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

// Wavy variant of CircleIndicatorShape. Active arc renders as a wavy stroked
// path via MD.PathArcWave; inactive arc stays flat per M3 spec.
MD.Shape {
    id: root
    property color strokeColor: MD.MProp.color.primary
    property color inactiveColor: "transparent"
    property real strokeWidth: 4

    // Match CircleIndicatorShape's radius so the wavy variant draws on the
    // same circle as the flat variant — wave peaks may extend slightly past
    // the bounds, but the parent control sets `clip: false` to allow that.
    readonly property int radius: height / 2
    readonly property vector2d center: Qt.vector2d(radius, radius)

    property real startAngle: 0
    property real endAngle: 0

    property real inactiveStartAngle: 0
    property real inactiveSweepAngle: 0

    // M3 spec circular-wavy defaults: amplitude 1.6dp, wavelength 15dp.
    property real waveLength: 15
    property real waveAmplitude: 1.6
    property real phase: 0

    asynchronous: false

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        strokeColor: root.inactiveColor
        strokeWidth: root.inactiveSweepAngle > 0 ? root.strokeWidth : 0

        PathAngleArc {
            centerX: root.center.x
            centerY: root.center.y
            radiusX: root.radius
            radiusY: root.radius
            startAngle: root.inactiveStartAngle
            sweepAngle: root.inactiveSweepAngle
            moveToStart: true
        }
    }

    ShapePath {
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin
        fillColor: "transparent"
        strokeColor: root.strokeColor
        strokeWidth: root.strokeWidth

        MD.PathArcWave {
            centerX: root.center.x
            centerY: root.center.y
            radius: root.radius
            startAngle: root.startAngle
            sweepAngle: root.endAngle - root.startAngle
            amplitude: root.waveAmplitude
            waveLength: root.waveLength
            phase: root.phase
        }
    }
}
