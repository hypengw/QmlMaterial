import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 800
    height: 640
    color: MD.Token.color.surface
    Column {
        x: 16
        y: 16
        spacing: 24
        MD.DateTextField {
            width: 360
            locale: Qt.locale("de_DE")
            dateFormat: "dd.MM.yyyy"
            value: new Date(2024, 1, 29)
        }
        MD.DatePicker {
            locale: Qt.locale("de_DE")
            selectedDate: new Date(2024, 1, 29)
        }
    }
    Column {
        x: 416
        y: 16
        spacing: 24
        MD.DateTextField {
            width: 360
            locale: Qt.locale("fr_FR")
            dateFormat: "d MMMM yyyy"
            value: new Date(2024, 1, 29)
        }
        MD.DatePicker {
            locale: Qt.locale("fr_FR")
            selectedDate: new Date(2024, 1, 29)
            minDate: new Date(2024, 1, 29, 12)
            maxDate: new Date(2024, 2, 2)
        }
    }
}
