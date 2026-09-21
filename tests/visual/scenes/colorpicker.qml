import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 600
    color: MD.MProp.color.surface_container_high

    MD.ColorPicker {
        anchors.centerIn: parent
        width: 360
        color: "#6750A4"
    }
}
