import QtQuick
import Qcm.Material as MD

MD.ProgressIndicatorShape {
    id: root
    circular: true
    legacyRadius: true
    property color inactiveColor: "transparent"
    readonly property real radius: height / 2
    readonly property vector2d center: Qt.vector2d(radius, radius)
    strokeColor: MD.MProp.color.primary
    trackColor: root.inactiveColor
}
