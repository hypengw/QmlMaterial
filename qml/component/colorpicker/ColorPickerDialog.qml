pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Dialog {
    id: control

    property color color
    property bool showAlpha: true
    standardButtons: T.DialogButtonBox.Cancel | T.DialogButtonBox.Ok

    signal acceptedColor(color c)

    title: "Pick color"
    buttonBoxPadding: 12

    contentItem: MD.VerticalFlickable {
        contentWidth: width
        implicitWidth: m_picker.implicitWidth
        contentHeight: m_picker.implicitHeight
        MD.ColorPicker {
            id: m_picker
            color: control.color
            showAlpha: control.showAlpha
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
    onAccepted: control.acceptedColor(m_picker.color)
}
