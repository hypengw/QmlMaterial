pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Row {
    id: root
    spacing: 16

    Item {
        id: firstOwner
        property bool menuExpanded: true
        width: Math.max(0, (root.width - root.spacing) / 2)
        height: 280
        MD.FABMenu {
            id: menu
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.min(implicitWidth, parent.width)
            height: parent.height
            expanded: firstOwner.menuExpanded
            onExpandedChangeRequested: value => firstOwner.menuExpanded = value
            MD.FABMenuItem {
                text: "First"
                icon.name: MD.Token.icon.stars
                onClicked: firstOwner.menuExpanded = false
            }
            MD.FABMenuItem {
                text: "Second"
                icon.name: MD.Token.icon.stars
                onClicked: firstOwner.menuExpanded = false
            }
        }
    }
    Item {
        id: scrollOwner
        property bool menuExpanded: false
        width: Math.max(0, (root.width - root.spacing) / 2)
        height: 280
        MD.FABMenu {
            id: scrollMenu
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.min(implicitWidth, parent.width)
            height: parent.height
            expanded: scrollOwner.menuExpanded
            onExpandedChangeRequested: value => scrollOwner.menuExpanded = value
            Repeater {
                model: ["Reply", "Reply all", "Forward", "Snooze", "Archive", "Label"]
                MD.FABMenuItem {
                    required property string modelData
                    text: modelData
                    icon.name: MD.Token.icon.star
                    onClicked: scrollOwner.menuExpanded = false
                }
            }
        }
    }
}
