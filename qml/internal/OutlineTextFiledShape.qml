import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Shape {
    id: root
    property alias bottomLineWidth: m_line.strokeWidth
    property real radius: 0

    MD.RoundPath {
        width: root.width
        height: root.height
        corners: MD.Util.corners(root.radius, 0)
    }

    ShapePath {
        id: m_line
        strokeWidth: 1
        strokeColor: "transparent"
        fillColor: "transparent"
        startX: strokeWidth / 2
        startY: root.height
        PathLine {
            x: root.width - m_line.strokeWidth / 2
            y: root.height
        }
    }
}
