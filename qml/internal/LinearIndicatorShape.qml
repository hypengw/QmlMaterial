import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property int strokeWidth: 4
    asynchronous: true

    property real x1: 0
    property real x2: 0

    property color c1: root.MD.MProp.color.secondary_container
    property color c2: root.MD.MProp.color.secondary_container
    property color c3: root.MD.MProp.color.secondary_container

    readonly property real w1: {
        if (x1 < strokeWidth) {
            return x1;
        } else if (x1 + strokeWidth > width) {
            return width - x1;
        } else {
            return strokeWidth;
        }
    }

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        strokeColor: root.c1
        strokeWidth: root.strokeWidth
        startX: 0

        PathLine {
            x: root.x1 - root.w1
        }
    }

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        strokeColor: root.c2
        strokeWidth: root.strokeWidth

        startX: root.x1 + root.w1

        PathLine {
            x: Math.min(root.x2 + root.w1, root.width)
        }
    }

    ShapePath {
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        strokeColor: root.c3
        strokeWidth: root.strokeWidth

        startX: Math.min(root.x2 + root.w1 + 2 * root.w1, root.width)

        PathLine {
            x: root.width
        }
    }
}
