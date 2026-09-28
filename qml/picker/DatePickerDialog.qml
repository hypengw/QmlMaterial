pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Dialog {
    id: control

    property alias selectionMode: m_picker.selectionMode
    property alias displayMode: m_picker.displayMode
    property alias showModeToggle: m_picker.showModeToggle
    property alias inputDateFormat: m_picker.inputDateFormat
    property alias selectedDate: m_picker.selectedDate
    property alias rangeStart: m_picker.rangeStart
    property alias rangeEnd: m_picker.rangeEnd
    property alias minDate: m_picker.minDate
    property alias maxDate: m_picker.maxDate
    property alias headlineFormat: m_picker.headlineFormat
    property alias monthTitleFormat: m_picker.monthTitleFormat
    readonly property alias selectionValid: m_picker.selectionValid

    signal acceptedDate(date d)
    signal acceptedRange(date start, date end)

    standardButtons: MD.Dialog.Cancel | MD.Dialog.Ok
    onAccepted: {
        if (!selectionValid)
            return;
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
    footer: MD.DialogButtonBox {
        id: buttons
        bottomPadding: control.buttonBoxPadding
        horizontalPadding: control.buttonBoxPadding
        visible: count > 0
        delegate: MD.Button {
            mdState.type: MD.Enum.BtText
            enabled: MD.DialogButtonBox.buttonRole !== MD.DialogButtonBox.AcceptRole || control.selectionValid
        }
    }
    horizontalPadding: 0
    topPadding: 0
    bottomPadding: 0

    contentItem: MD.VerticalFlickable {
        contentWidth: width
        implicitWidth: m_picker.implicitWidth
        contentHeight: m_picker.implicitHeight
        MD.DatePicker {
            id: m_picker
            locale: control.locale
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
