pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

Lite.Box {
    id: root
    required property var button
    required property MD.StateButtonBase mdState
    required property MD.ButtonMotion motion
    required property int iconStyle
    required property MD.typescale typescale
    alignment: Qt.AlignCenter
    opacity: motion.contentOpacity
    Lite.Row {
        transform: Translate {
            x: root.button.MD.ButtonGroupContainer.connected ? root.mdState.groupOpticalOffset(root.motion.corners) : 0
        }
        width: root.button.MD.ButtonGroupContainer.grouped ? implicitWidth : Math.min(implicitWidth, parent.width)
        height: Math.min(implicitHeight, parent.height)
        alignment: Qt.AlignHCenter | Qt.AlignVCenter
        spacing: root.button.spacing
        MD.IconView {
            visible: root.iconStyle != MD.Enum.TextOnly && !root.button.icon.empty
            icon: root.button.icon
        }
        MD.Label {
            visible: root.iconStyle != MD.Enum.IconOnly
            text: root.button.text
            color: root.motion.textColor
            useTypescale: false
            font: root.button.font
            lineHeight: root.typescale.line_height
            wrapMode: Text.NoWrap
            Lite.Layout.fillWidth: true
        }
    }
}
