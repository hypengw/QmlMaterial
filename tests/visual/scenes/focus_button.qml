import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 200
    color: "#F5F5F5"

    MD.Button {
        id: btn
        anchors.centerIn: parent
        text: "Focused"

        MD.FocusIndicator {
            parent: btn.background
            corners: btn.background.corners
            active: true
        }
    }
}
