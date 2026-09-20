import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 200
    height: 200
    color: "#F5F5F5"

    MD.RadioButton {
        id: rb
        anchors.centerIn: parent
        text: "Option"
        checked: true

        MD.FocusIndicator {
            parent: rb.indicator.children[1]
            corners: MD.Util.corners(20)
            active: true
        }
    }
}
