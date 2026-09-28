pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.TextField {
    id: control

    property date value: new Date()
    property alias dateFormat: m_validator.dateFormat
    property alias locale: m_validator.locale
    property alias minDate: m_validator.minDate
    property alias maxDate: m_validator.maxDate
    property bool __dateComplete: false
    Component.onCompleted: {
        __dateComplete = true;
        _syncText();
    }

    signal modified(date d)

    placeholderText: m_validator.inputFormat
    text: m_validator.formatDate(value)
    inputMethodHints: Qt.ImhDate
    selectByMouse: true

    validator: MD.DateValidator {
        id: m_validator
    }

    function _syncText() {
        if (__dateComplete)
            text = m_validator.formatDate(value);
    }
    onValueChanged: _syncText()
    onDateFormatChanged: _syncText()
    onLocaleChanged: _syncText()
    onEditingFinished: {
        const d = m_validator.parse(text);
        if (!isNaN(d.getTime())) {
            value = d;
            _syncText();
            modified(d);
        }
    }

    trailing: MD.StandardIconButton {
        icon.width: 22
        icon.height: 22
        implicitBackgroundSize: 0
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 8
        icon.name: MD.Token.icon.calendar_today
        enabled: !control.readOnly
        onClicked: m_dialog.open()
    }

    MD.DatePickerDialog {
        id: m_dialog
        parent: control.MD.Overlay.overlay
        selectionMode: MD.DatePicker.SelectionMode.Single
        selectedDate: control.value
        locale: control.locale
        minDate: control.minDate
        maxDate: control.maxDate
        onAboutToShow: selectedDate = control.value
        onAcceptedDate: function (d) {
            control.value = d;
            control.modified(d);
        }
    }
}
