pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ColorPickerButtonBase {
    id: control

    property int radius: 8

    implicitWidth: 80
    implicitHeight: 32

    padding: 0
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    hoverEnabled: enabled

    contentItem: Item {}

    background: Item {
        anchors.fill: parent

        MD.Rectangle {
            id: m_swatch
            anchors.left: parent.left
            anchors.right: m_trail.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.rightMargin: 2
            color: control.color
            corners: MD.Util.corners(control.radius, 0, control.radius, 0)
            clip: true

            MD.Ripple {
                anchors.fill: parent
                corners: parent.corners
                interactionSource: control.interactionSource
                color: MD.HctUtil.toneOf(control.color) > 50 ? "#000000" : "#ffffff"
            }
        }

        MD.Rectangle {
            id: m_trail
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: height + 6
            color: MD.HctUtil.paletteTone(control.color, MD.MProp.color.mode === MD.Enum.Dark ? 30 : 90)
            corners: MD.Util.corners(0, control.radius, 0, control.radius)
            clip: true

            MD.Ripple {
                anchors.fill: parent
                corners: parent.corners
                interactionSource: control.interactionSource
                color: MD.HctUtil.paletteTone(control.color, MD.MProp.color.mode === MD.Enum.Dark ? 80 : 30)
            }

            MD.Icon {
                anchors.centerIn: parent
                name: MD.Token.icon.colorize
                size: 16
                color: MD.HctUtil.paletteTone(control.color, MD.MProp.color.mode === MD.Enum.Dark ? 80 : 30)
            }
        }
    }
}
