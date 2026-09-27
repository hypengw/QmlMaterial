pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Column {
    id: root
    spacing: 20

    component ChoiceIcon: MD.IconButton {
        icon.name: MD.Token.icon.star
        checkable: true
        mdState.type: MD.Enum.IBtFilled
    }

    MD.Label {
        text: "Standard"
        typescale: MD.Token.typescale.title_medium
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        MD.IconButton {
            icon.name: "bluetooth"
            mdState.type: MD.Enum.IBtFilled
            mdState.widthMode: MD.Enum.NarrowWidth
        }
        MD.IconButton {
            icon.name: "alarm"
            mdState.type: MD.Enum.IBtFilled
        }
        MD.IconButton {
            icon.name: "link"
            mdState.type: MD.Enum.IBtFilled
        }
        MD.IconButton {
            icon.name: "wifi"
            mdState.type: MD.Enum.IBtFilled
            mdState.widthMode: MD.Enum.WideWidth
        }
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        MD.Button {
            text: "Label"
            mdState.type: MD.Enum.BtFilledTonal
        }
        MD.Button {
            text: "Edit"
            icon.name: MD.Token.icon.edit
            mdState.type: MD.Enum.BtFilled
            mdState.isRound: false
        }
        MD.IconButton {
            icon.name: MD.Token.icon.delete
            mdState.type: MD.Enum.IBtFilledTonal
        }
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        ChoiceIcon {}
        MD.Button {
            text: "Label"
            icon.name: MD.Token.icon.star
            checkable: true
            checked: true
            mdState.type: MD.Enum.BtFilled
        }
        ChoiceIcon {}
    }
    MD.Label {
        text: "Connected"
        typescale: MD.Token.typescale.title_medium
    }
    MD.ButtonGroup {
        id: fileSelection
    }
    MD.ButtonGroupContainer {
        objectName: "fileButtons"
        width: Math.max(0, root.width)
        variant: MD.ButtonGroupContainer.Connected
        MD.Button {
            text: "My files"
            checkable: true
            checked: true
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: fileSelection
            MD.ButtonGroupContainer.weight: 1
        }
        MD.Button {
            text: "Shared"
            checkable: true
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: fileSelection
            MD.ButtonGroupContainer.weight: 1
        }
        MD.Button {
            text: "Computers"
            checkable: true
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: fileSelection
            MD.ButtonGroupContainer.weight: 1
        }
    }
    MD.ButtonGroup {
        id: singleSelection
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        variant: MD.ButtonGroupContainer.Connected
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
            checked: true
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
        }
    }
    MD.ButtonGroup {
        id: multipleSelection
        exclusive: false
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        variant: MD.ButtonGroupContainer.Connected
        MD.Button {
            text: "One"
            icon.name: MD.Token.icon.star
            checkable: true
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: multipleSelection
        }
        MD.Button {
            text: "Two"
            icon.name: MD.Token.icon.star
            checkable: true
            checked: true
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: multipleSelection
        }
        MD.Button {
            text: "Three"
            checkable: true
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: multipleSelection
        }
    }
    MD.ButtonGroupContainer {
        width: Math.max(0, root.width)
        variant: MD.ButtonGroupContainer.Connected
        ChoiceIcon {
            MD.ButtonGroupContainer.weight: 1
            MD.ButtonGroupContainer.minimumWidth: 40
        }
        MD.Button {
            text: "Label"
            icon.name: MD.Token.icon.star
            checkable: true
            checked: true
            mdState.type: MD.Enum.BtFilled
            MD.ButtonGroupContainer.weight: 1
            MD.ButtonGroupContainer.minimumWidth: implicitWidth
        }
        ChoiceIcon {
            MD.ButtonGroupContainer.weight: 1
            MD.ButtonGroupContainer.minimumWidth: 40
        }
    }
}
