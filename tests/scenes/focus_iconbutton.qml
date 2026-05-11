import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 200
    height: 200
    color: "#F5F5F5"

    MD.IconButton {
        id: btn
        anchors.centerIn: parent
        icon.name: "favorite"

        MD.FocusIndicator {
            parent: btn.background
            corners: btn.background.corners
            active: true
        }
    }
}
