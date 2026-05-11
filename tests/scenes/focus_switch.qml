import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 200
    height: 200
    color: "#F5F5F5"

    MD.Switch {
        id: sw
        anchors.centerIn: parent
        checked: true

        MD.FocusIndicator {
            parent: sw.indicator.children[1]
            corners: MD.Util.corners(parent.width / 2)
            active: true
        }
    }
}
