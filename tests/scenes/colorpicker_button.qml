import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 300
    height: 120
    color: MD.MProp.color.surface_container_high

    Row {
        anchors.centerIn: parent
        spacing: 16

        MD.ColorPickerButton {
            color: "#00c8c0"
        }
        MD.ColorPickerButton {
            color: "#6750A4"
            width: 100
        }
        MD.ColorPickerButton {
            color: "#F44336"
            width: 60
            height: 28
        }
    }
}
