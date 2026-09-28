import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 1440
    height: 500
    color: MD.Token.color.surface

    MD.DatePicker {
        width: 360
        selectedDate: new Date(2024, 1, 29)
        locale: Qt.locale("de_DE")
    }
    MD.DatePicker {
        x: 360
        width: 360
        locale: Qt.locale("fr_FR")
        selectionMode: MD.DatePicker.Range
        displayMode: MD.DatePicker.Input
        rangeStart: new Date(2024, 1, 29)
        rangeEnd: new Date(2024, 2, 2)
    }
    MD.DatePicker {
        x: 720
        width: 360
        selectedDate: new Date(2024, 1, 29)
        minDate: new Date(2020, 0, 1)
        maxDate: new Date(2030, 11, 31)
        Component.onCompleted: __navigation = 1
    }
    MD.DatePicker {
        x: 1080
        width: 360
        locale: Qt.locale("fr_FR")
        selectedDate: new Date(2024, 1, 29)
        minDate: new Date(2024, 1, 1)
        maxDate: new Date(2024, 8, 30)
        Component.onCompleted: __navigation = 2
    }
}
