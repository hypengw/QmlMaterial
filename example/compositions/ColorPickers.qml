pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 400
    implicitHeight: content.implicitHeight
    Connections {
        target: picker
        function onColorChanged() {
            colorButton.color = picker.color;
        }
    }
    Column {
        id: content
        width: parent.width
        spacing: 16
        MD.ColorPickerButton {
            id: colorButton
            objectName: 'exampleColorButton'
            width: 120
            color: picker.color
            onAccepted: color => picker.color = color
        }
        MD.ColorPicker {
            id: picker
            objectName: 'exampleColorPicker'
            width: parent.width
            showHeader: false
            color: '#6750A4'
        }
    }
}
