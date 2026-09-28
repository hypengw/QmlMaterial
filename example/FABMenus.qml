pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Column {
    id: root
    spacing: 16

    MD.Label {
        text: "FAB menu"
        typescale: MD.Token.typescale.title_medium
    }
    Item {
        width: root.width
        height: 240
        MD.FABMenu {
            id: menu
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.min(implicitWidth, parent.width)
            height: parent.height
            expanded: true
            MD.FABMenuItem {
                text: "First"
                icon.name: MD.Token.icon.stars
                onClicked: menu.close()
            }
            MD.FABMenuItem {
                text: "Second"
                icon.name: MD.Token.icon.stars
                onClicked: menu.close()
            }
        }
    }
    MD.Label {
        text: "Scrollable FAB menu"
        typescale: MD.Token.typescale.title_medium
    }
    Item {
        width: root.width
        height: 280
        MD.FABMenu {
            id: scrollMenu
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.min(implicitWidth, parent.width)
            height: parent.height
            Repeater {
                model: ["Reply", "Reply all", "Forward", "Snooze", "Archive", "Label"]
                MD.FABMenuItem {
                    required property string modelData
                    text: modelData
                    icon.name: MD.Token.icon.star
                    onClicked: scrollMenu.close()
                }
            }
        }
    }
}
