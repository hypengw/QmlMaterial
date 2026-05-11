import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 300
    height: 200
    color: "#F5F5F5"

    MD.AssistChip {
        id: chip
        anchors.centerIn: parent
        text: "Focused Chip"

        MD.FocusIndicator {
            parent: chip.background
            corners: MD.Util.corners(chip.background.radius)
            active: true
        }
    }
}
