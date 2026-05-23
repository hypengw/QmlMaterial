pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Dialog {
    id: control

    property alias selectionMode: m_picker.selectionMode
    property alias selectedDate: m_picker.selectedDate
    property alias rangeStart: m_picker.rangeStart
    property alias rangeEnd: m_picker.rangeEnd
    property alias minDate: m_picker.minDate
    property alias maxDate: m_picker.maxDate

    signal acceptedDate(date d)
    signal acceptedRange(date start, date end)

    standardButtons: T.DialogButtonBox.Cancel | T.DialogButtonBox.Ok
    onAccepted: {
        if (m_picker.selectionMode === MD.DatePicker.SelectionMode.Single) {
            control.acceptedDate(m_picker.selectedDate);
        } else if (m_picker.rangeStart && m_picker.rangeEnd) {
            control.acceptedRange(m_picker.rangeStart, m_picker.rangeEnd);
        }
    }

    title: ""
    implicitHeight: m_picker.implicitHeight + 96
    buttonBoxPadding: 12

    header: null
    horizontalPadding: 0
    topPadding: 0
    bottomPadding: 0

    contentItem: MD.VerticalFlickable {
        contentWidth: width
        implicitWidth: m_picker.implicitWidth
        contentHeight: m_picker.implicitHeight
        MD.DatePicker {
            id: m_picker
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
