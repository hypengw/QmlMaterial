import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Shape {
    id: shape
    property int strokeWidth: 4
    property int radius: 16
    property real progress: 0
    readonly property size center: Qt.size(radius + strokeWidth / 2, radius + strokeWidth / 2)
    width: center.width * 2
    height: center.height * 2

    layer.enabled: true
    layer.mipmap: true
    layer.smooth: true
    layer.samples: 4

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: shape.center.width
        startY: 0
        strokeColor: MD.Util.transparent(MD.Token.color.primary, 0.3)
        strokeWidth: shape.strokeWidth

        PathAngleArc {
            centerX: shape.center.width
            centerY: shape.center.height
            radiusX: 16
            radiusY: radiusX
            startAngle: -90
            sweepAngle: 360
        }
    }

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        startX: shape.radius
        startY: 0
        strokeColor: MD.Token.color.primary
        strokeWidth: shape.strokeWidth

        PathAngleArc {
            centerX: shape.center.width
            centerY: shape.center.height
            radiusX: 16
            radiusY: radiusX
            startAngle: -90
            sweepAngle: 360 * shape.progress
        }
    }
}
