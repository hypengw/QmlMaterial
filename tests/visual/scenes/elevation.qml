import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 300
    color: "#aaaaaa"

    MD.Rectangle {
        id: box
        anchors.centerIn: parent
        width: 160
        height: 120
        radius: 12
        color: "#ffffff"

        MD.Elevation {
            anchors.fill: parent
            elevation: MD.Token.elevation.level5
            corners: box.corners
            z: -1
        }
    }
}
