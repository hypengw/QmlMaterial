import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 1060
    height: 600
    color: MD.Token.color.surface
    MD.TimePicker {
        x: 10
        y: 10
        time.hourFormat: MD.TimeState.Hour12
        time.hour: 10
        time.minute: 30
    }
    MD.TimePicker {
        x: 350
        y: 10
        time.hourFormat: MD.TimeState.Hour24
        time.hour: 18
        time.minute: 45
    }
    MD.TimePicker {
        x: 690
        y: 10
        locale: Qt.locale("ar_EG")
        layoutDirection: Qt.RightToLeft
        time.hourFormat: MD.TimeState.Hour12
        time.hour: 13
        time.minute: 5
        time.displayMode: MD.TimeState.Input
    }
    MD.TimeTextField {
        x: 714
        y: 300
        width: 240
        hourFormat: MD.TimeState.Hour24
        value: 1095
    }
    MD.TimeInput {
        x: 714
        y: 390
        enabled: false
        time.hourFormat: MD.TimeState.Hour24
        time.hour: 9
        time.minute: 30
    }
}
