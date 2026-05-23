pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property color color: "#6750A4"
    property bool showAlpha: true
    property int radius: 8

    signal accepted(color c)

    implicitWidth: 80
    implicitHeight: 32

    padding: 0
    leftInset: 0; rightInset: 0; topInset: 0; bottomInset: 0
    hoverEnabled: enabled

    onClicked: m_dialog.open()

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
                pressX: control.pressX - parent.x
                pressY: control.pressY - parent.y
                pressed: control.pressed
                stateOpacity: control.hovered ? 0.08 : 0
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
                pressX: control.pressX - parent.x
                pressY: control.pressY - parent.y
                pressed: control.pressed
                stateOpacity: control.hovered ? 0.08 : 0
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

    MD.ColorPickerDialog {
        id: m_dialog
        parent: T.Overlay.overlay
        color: control.color
        showAlpha: control.showAlpha
        onAcceptedColor: function (c) {
            control.color = c;
            control.accepted(c);
        }
    }
}
