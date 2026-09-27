import QtQuick
import Qcm.Material as MD
import "../../../example" as Demo

Rectangle {
    id: root
    width: 780
    height: 720
    color: "#fffbfe"

    Demo.ButtonGroups {
        id: ltrGroups
        x: 24
        y: 24
        width: Math.min(340, root.width - 48)
    }
    Demo.ButtonGroups {
        x: 416
        y: 24
        width: 340
        visible: root.width >= 760
        LayoutMirroring.enabled: true
        LayoutMirroring.childrenInherit: true
    }
    MD.ButtonGroupContainer {
        id: pressedGroup
        x: 24
        y: ltrGroups.y + ltrGroups.height + 32
        MD.Button {
            text: "First"
            mdState.type: MD.Enum.BtFilled
        }
        MD.Button {
            text: "Pressed"
            down: true
            mdState.type: MD.Enum.BtFilled
        }
        MD.Button {
            text: "Last"
            mdState.type: MD.Enum.BtFilled
        }
    }
    MD.ButtonGroupContainer {
        x: 24
        y: pressedGroup.y + pressedGroup.height + 24
        variant: MD.ButtonGroupContainer.Connected
        MD.Button {
            text: "Disabled"
            enabled: false
            mdState.type: MD.Enum.BtFilledTonal
        }
        MD.Button {
            id: focusButton
            text: "Focus"
            mdState.type: MD.Enum.BtFilledTonal
            MD.FocusIndicator {
                parent: focusButton.background
                corners: focusButton.background.corners
                active: true
            }
        }
        MD.IconButton {
            icon.name: MD.Token.icon.star
            down: true
            mdState.type: MD.Enum.IBtFilled
        }
    }
}
