pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 360
    implicitHeight: content.implicitHeight
    readonly property alias time: timeField.time

    function openDialog() {
        timeField.openPicker();
    }

    Column {
        id: content
        width: parent.width
        spacing: 16
        MD.TimeTextField {
            id: timeField
            objectName: 'exampleTimeField'
            width: Math.min(240, root.width)
            x: (parent.width - width) / 2
            value: 10 * 60 + 30
            hourFormat: MD.TimeState.Hour12
            time.objectName: 'pickerTime'
            time.displayMode: root.width < 320 ? MD.TimeState.Input : MD.TimeState.Clock
        }
        MD.SegmentedButtonGroup {
            objectName: 'pickerHourFormatChoice'
            x: (parent.width - width) / 2
            size: MD.Enum.XS
            Accessible.name: 'Hour format'
            MD.SegmentedButton {
                objectName: 'picker12HourChoice'
                width: 104
                text: '12h'
                checked: root.time.hourFormat === MD.TimeState.Hour12
                onCheckedChangeRequested: value => {
                    if (value)
                        root.time.hourFormat = MD.TimeState.Hour12;
                }
            }
            MD.SegmentedButton {
                objectName: 'picker24HourChoice'
                width: 104
                text: '24h'
                checked: root.time.hourFormat === MD.TimeState.Hour24
                onCheckedChangeRequested: value => {
                    if (value)
                        root.time.hourFormat = MD.TimeState.Hour24;
                }
            }
        }
        MD.TimePicker {
            id: picker
            objectName: 'exampleTimePicker'
            width: Math.min(implicitWidth, root.width)
            x: (parent.width - width) / 2
            time: root.time
            background: Rectangle {
                objectName: 'exampleTimePickerBackground'
                radius: MD.Token.shape.corner.extra_large
                color: picker.MD.MProp.color.surface_container_high
            }
        }
    }
}
