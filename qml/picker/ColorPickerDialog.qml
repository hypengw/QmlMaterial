pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Dialog {
    id: control

    property color color
    property bool showAlpha: true
    readonly property color selectedColor: m_picker.color
    standardButtons: MD.Dialog.Cancel | MD.Dialog.Ok

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
}
