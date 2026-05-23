pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.TextField {
    id: control

    property date value: new Date()
    property string dateFormat: "yyyy-MM-dd"

    signal modified(date d)

    placeholderText: dateFormat
    text: Qt.formatDate(value, dateFormat)
    inputMethodHints: Qt.ImhDate
    selectByMouse: true

    validator: RegularExpressionValidator {
        regularExpression: /^\d{4}[-/]\d{1,2}[-/]\d{1,2}$/
    }

    function _parse(s) {
        const norm = s.replace(/\//g, "-");
        const m = norm.match(/^(\d{4})-(\d{1,2})-(\d{1,2})$/);
        if (!m) return null;
        const y = parseInt(m[1]), mo = parseInt(m[2]) - 1, da = parseInt(m[3]);
        if (mo < 0 || mo > 11 || da < 1 || da > 31) return null;
        const d = new Date(y, mo, da);
        if (d.getFullYear() !== y || d.getMonth() !== mo || d.getDate() !== da) return null;
        return d;
    }
    onEditingFinished: {
        const d = _parse(text);
        if (d) {
            value = d;
            modified(d);
        } else {
            text = Qt.formatDate(value, dateFormat);
        }
    }

    trailing: MD.SmallIconButton {
        anchors.right: parent?.right
        anchors.verticalCenter: parent?.verticalCenter
        anchors.rightMargin: 8
        icon.name: MD.Token.icon.calendar_today
        onClicked: m_dialog.open()
    }

    MD.DatePickerDialog {
        id: m_dialog
        parent: T.Overlay.overlay
        selectionMode: MD.DatePicker.SelectionMode.Single
        selectedDate: control.value
        onAcceptedDate: function (d) {
            control.value = d;
            control.modified(d);
        }
    }
}
