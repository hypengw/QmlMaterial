import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 400
    height: 560
    color: MD.MProp.color.surface_container_high

    MD.DatePicker {
        anchors.centerIn: parent
        showHeader: false
        selectionMode: MD.DatePicker.SelectionMode.Single
        selectedDate: new Date(2026, 4, 15)
        year: 2026
        month: 4
    }
}
