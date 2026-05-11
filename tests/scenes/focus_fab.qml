import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 200
    height: 200
    color: "#F5F5F5"

    MD.FAB {
        id: btn
        anchors.centerIn: parent
        icon.name: "add"

        MD.FocusIndicator {
            parent: btn.background
            corners: MD.Util.corners(btn.background.radius)
            active: true
        }
    }
}
