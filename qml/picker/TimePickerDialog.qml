pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Dialog {
    id: control
    property alias value: initialTime.value
    property alias hourFormat: draft.hourFormat
    property alias displayMode: draft.displayMode
    readonly property alias time: draft
    MD.TimeState {
        id: initialTime
    }
    MD.TimeState {
        id: draft
        locale: control.locale
    }
    signal acceptedTime(int hour, int minute)
    header: null
    padding: 0
    horizontalPadding: 0
    topPadding: 0
    bottomPadding: 0
    buttonBoxPadding: 12
    standardButtons: MD.Dialog.Cancel | MD.Dialog.Ok
    acceptEnabled: time.acceptableInput
    width: Math.min(implicitWidth, parent ? parent.width : implicitWidth)
    height: Math.min(implicitHeight, parent ? parent.height : implicitHeight)
    onAboutToShow: {
        time.value = value;
        time.discardInput();
        time.selection = MD.TimeState.Hours;
    }
    onAccepted: {
        const hour = time.hour;
        const minute = time.minute;
        value = time.value;
        acceptedTime(hour, minute);
    }
    footer: MD.DialogButtonBox {
        bottomPadding: control.buttonBoxPadding
        horizontalPadding: control.buttonBoxPadding
        delegate: MD.Button {
            mdState.type: MD.Enum.BtText
            enabled: MD.DialogButtonBox.buttonRole !== MD.DialogButtonBox.AcceptRole || control.acceptEnabled
        }
    }
    contentItem: MD.VerticalFlickable {
        contentWidth: width
        contentHeight: picker.implicitHeight
        implicitWidth: picker.implicitWidth
        implicitHeight: picker.implicitHeight
        MD.TimePicker {
            id: picker
            time: draft
            width: parent.width
            locale: control.locale
        }
    }
}
