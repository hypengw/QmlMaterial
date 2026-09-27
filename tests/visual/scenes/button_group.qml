import QtQuick
import Qcm.Material as MD
import "../../../example" as Demo

Rectangle {
    id: root
    width: 780
    height: 640
    color: "#fffbfe"

    Demo.ButtonGroups {
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
        x: 24
        y: 520
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
        y: 584
        variant: MD.ButtonGroupContainer.Connected
        MD.Button {
            text: "Disabled"
            enabled: false
            mdState.type: MD.Enum.BtTonal
        }
        MD.Button {
            id: focusButton
            text: "Focus"
            mdState.type: MD.Enum.BtTonal
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
