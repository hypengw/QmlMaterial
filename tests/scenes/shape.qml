import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 300
    color: "#E0E0E0"

    MD.Rectangle {
        id: box
        anchors.centerIn: parent
        width: 160
        height: 100
        color: "#FFFFFF"
        corners: MD.Util.corners(20, 0, 20, 0)

        MD.Elevation {
            anchors.fill: parent
            elevation: 2
            corners: box.corners
            z: -1
        }
    }
}
