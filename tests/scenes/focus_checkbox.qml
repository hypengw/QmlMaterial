import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 200
    height: 200
    color: "#F5F5F5"

    MD.CheckBox {
        id: cb
        anchors.centerIn: parent
        text: "Accept"
        checked: true

        MD.FocusIndicator {
            parent: cb.indicator.children[1]
            corners: MD.Util.corners(20)
            active: true
        }
    }
}
