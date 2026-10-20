pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Column {
    id: root
    spacing: 20

    component ChoiceIcon: MD.IconToggleButton {
        icon.name: MD.Token.icon.star
        property bool value: false
        checked: value
        onCheckedChangeRequested: next => {
            if (!MD.ButtonGroup.group)
                value = next;
        }
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
        MD.ToggleButton {
            property bool value: true
            checked: value
            onCheckedChangeRequested: next => value = next
            text: "Label"
            icon.name: MD.Token.icon.star
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
        checkedButtons: [firstFile]
    }
    MD.ButtonGroupContainer {
        objectName: "fileButtons"
        width: Math.max(0, root.width)
        variant: MD.ButtonGroupContainer.Connected
        MD.ToggleButton {
            id: firstFile
            text: "My files"
            checked: MD.ButtonGroup.checked
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: fileSelection
            MD.ButtonGroupContainer.weight: 1
        }
        MD.ToggleButton {
            text: "Shared"
            checked: MD.ButtonGroup.checked
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: fileSelection
            MD.ButtonGroupContainer.weight: 1
        }
        MD.ToggleButton {
            text: "Computers"
            checked: MD.ButtonGroup.checked
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: fileSelection
            MD.ButtonGroupContainer.weight: 1
        }
    }
    MD.ButtonGroup {
        id: singleSelection
        checkedButtons: [firstChoice]
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        variant: MD.ButtonGroupContainer.Connected
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
            checked: MD.ButtonGroup.checked
        }
        ChoiceIcon {
            id: firstChoice
            MD.ButtonGroup.group: singleSelection
            checked: MD.ButtonGroup.checked
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
            checked: MD.ButtonGroup.checked
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
            checked: MD.ButtonGroup.checked
        }
        ChoiceIcon {
            MD.ButtonGroup.group: singleSelection
            checked: MD.ButtonGroup.checked
        }
    }
    MD.ButtonGroup {
        id: multipleSelection
        checkedButtons: [secondMultiple]
        exclusive: false
    }
    MD.ButtonGroupContainer {
        anchors.horizontalCenter: parent.horizontalCenter
        variant: MD.ButtonGroupContainer.Connected
        MD.ToggleButton {
            text: "One"
            icon.name: MD.Token.icon.star
            checked: MD.ButtonGroup.checked
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: multipleSelection
        }
        MD.ToggleButton {
            id: secondMultiple
            text: "Two"
            icon.name: MD.Token.icon.star
            checked: MD.ButtonGroup.checked
            mdState.type: MD.Enum.BtFilledTonal
            MD.ButtonGroup.group: multipleSelection
        }
        MD.ToggleButton {
            text: "Three"
            checked: MD.ButtonGroup.checked
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
        MD.ToggleButton {
            property bool value: true
            checked: value
            onCheckedChangeRequested: next => value = next
            text: "Label"
            icon.name: MD.Token.icon.star
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
