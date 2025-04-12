import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property int strokeWidth: 4
    readonly property int radius: height / 2
    readonly property vector2d center: Qt.vector2d(radius, radius)
    property real startAngle: 0
    property real endAngle: 0
    asynchronous: true

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: root.radius
        startY: 0
        strokeColor: root.MD.MProp.color.primary
        strokeWidth: root.strokeWidth

        PathAngleArc {
            centerX: root.center.x
            centerY: root.center.y
            radiusX: 16
            radiusY: radiusX
            startAngle: root.startAngle
            sweepAngle: root.endAngle - root.startAngle
        }
    }
}
