pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.TextField {
    id: control
    readonly property MD.TimeState time: MD.TimeState {
        id: selectedTime
    }
    property alias value: selectedTime.value
    property alias hourFormat: selectedTime.hourFormat
    property alias locale: selectedTime.locale
    signal modified(int value)
    text: time.displayText
    placeholderText: rules.time.inputFormat
    inputMethodHints: Qt.ImhTime
    selectByMouse: true
    validator: MD.TimeValidator {
        id: rules
        time: control.time
    }
    onEditingFinished: {
        const parsed = rules.parse(text);
        if (parsed < 0)
            return;
        time.value = parsed;
        text = time.displayText;
        modified(parsed);
    }
    Connections {
        target: control.time
        function onChanged() {
            control.text = control.time.displayText;
        }
    }
    trailing: MD.StandardIconButton {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 8
        icon.name: "schedule"
        implicitBackgroundSize: 0
        enabled: !control.readOnly
        onClicked: {
            dialog.value = control.value;
            dialog.open();
        }
    }
    MD.TimePickerDialog {
        id: dialog
        parent: control.MD.Overlay.overlay
        locale: control.locale
        hourFormat: control.hourFormat
        onAcceptedTime: {
            const selected = value;
            control.value = selected;
            control.modified(selected);
        }
    }
}
