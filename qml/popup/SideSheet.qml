pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.SideSheetBase {
    id: control
    property color color: MD.Token.color.surface
    property int elevation: MD.Token.elevation.level0
    property int radius: detached ? MD.Token.shape.corner.large : 0

    sheetItem.contentItem.clip: true
    sheetItem.background: MD.ElevationRectangle {
        color: control.color
        elevation: control.elevation
        corners: control.detached ? MD.Util.corners(control.radius) : control.effectiveEdge === Qt.LeftEdge ? MD.Util.corners(0, control.radius, 0, control.radius) : MD.Util.corners(control.radius, 0, control.radius, 0)
    }
}
