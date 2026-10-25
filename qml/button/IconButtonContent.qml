pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    required property var button
    required property MD.StateIconButton mdState
    required property MD.ButtonMotion motion
    implicitWidth: button.icon.width
    implicitHeight: button.icon.height
    opacity: motion.contentOpacity
    MD.IconView {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: root.mdState.groupOpticalOffset(root.motion.corners)
        icon: root.button.icon
    }
}
