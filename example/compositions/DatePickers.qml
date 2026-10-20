pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 360
    implicitHeight: content.implicitHeight
    Connections {
        target: picker
        function onSelectedDateChanged() {
            dateField.value = picker.selectedDate;
        }
    }
    Column {
        id: content
        width: parent.width
        spacing: 16
        MD.DateTextField {
            id: dateField
            objectName: 'exampleDateField'
            width: Math.min(240, parent.width)
            x: (parent.width - width) / 2
            locale: picker.locale
            dateFormat: ''
            value: picker.selectedDate
            visible: picker.selectionMode === MD.DatePicker.Single
            onModified: date => picker.selectedDate = date
        }
        MD.SegmentedButtonGroup {
            objectName: 'pickerDateSelectionChoice'
            x: (parent.width - width) / 2
            size: MD.Enum.XS
            Accessible.name: 'Date selection'
            MD.SegmentedButton {
                objectName: 'pickerSingleDateChoice'
                width: 104
                text: 'Single'
                checked: picker.selectionMode === MD.DatePicker.Single
                onCheckedChangeRequested: value => {
                    if (value)
                        picker.selectionMode = MD.DatePicker.Single;
                }
            }
            MD.SegmentedButton {
                objectName: 'pickerDateRangeChoice'
                width: 104
                text: 'Range'
                checked: picker.selectionMode === MD.DatePicker.Range
                onCheckedChangeRequested: value => {
                    if (value)
                        picker.selectionMode = MD.DatePicker.Range;
                }
            }
        }
        MD.Flickable {
            id: viewport
            objectName: 'exampleDatePickerViewport'
            flickableDirection: Flickable.HorizontalFlick
            MD.ScrollBarBase.horizontal: MD.ScrollBar {}
            width: Math.min(picker.implicitWidth, root.width)
            x: (parent.width - width) / 2
            height: picker.implicitHeight
            contentWidth: picker.width
            contentHeight: height
            MD.DatePicker {
                id: picker
                onSelectedDateChangeRequested: date => selectedDate = date
                onRangeChangeRequested: (start, end) => dateState.setRange(start, end)
                objectName: 'exampleDatePicker'
                width: displayMode === MD.DatePicker.Calendar ? implicitWidth : viewport.width
                displayMode: root.width < implicitWidth ? MD.DatePicker.Input : MD.DatePicker.Calendar
                selectedDate: new Date()
                background: Rectangle {
                    objectName: 'exampleDatePickerBackground'
                    radius: MD.Token.shape.corner.extra_large
                    color: picker.MD.MProp.color.surface_container_high
                }
            }
        }
    }
}
