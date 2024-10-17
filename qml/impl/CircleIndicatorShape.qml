import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property int strokeWidth: 4
    readonly property int radius: height / 2
    readonly property vector2d center: Qt.vector2d(radius, radius)
    property real sweepAngle: 0
    property real startAngle: -90
    asynchronous: false

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: root.radius
        startY: 0
        strokeColor: root.MD.MatProp.color.primary
        strokeWidth: root.strokeWidth

        PathAngleArc {
            centerX: root.center.x
            centerY: root.center.y
            radiusX: 16
            radiusY: radiusX
            startAngle: root.startAngle
            sweepAngle: root.sweepAngle
        }
    }
}
