import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 640
    height: 320
    color: MD.Token.color.surface
    MD.FABMenu {
        anchors.right: parent.horizontalCenter
        anchors.bottom: parent.bottom
        height: parent.height
        expanded: true
        animationsEnabled: false
        MD.FABMenuItem {
            text: "First"
            icon.name: "stars"
        }
        MD.FABMenuItem {
            text: "Second"
            icon.name: "stars"
        }
    }
    MD.FABMenu {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: parent.height
        expanded: true
        animationsEnabled: false
        MD.FABMenuItem {
            text: "First"
            icon.name: "stars"
            widthProgress: 0.65
            alphaProgress: 0.65
        }
        MD.FABMenuItem {
            text: "Second"
            icon.name: "stars"
        }
    }
}
