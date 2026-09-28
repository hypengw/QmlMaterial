import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 280
    height: 480
    color: MD.Token.color.surface
    MD.TimePickerDialog {
        parent: root
        value: 13 * 60 + 25
        hourFormat: MD.TimeState.Hour12
        enter: null
        exit: null
        Component.onCompleted: open()
    }
}
