import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 480
    height: 320
    color: "#F5F5F5"

    Column {
        anchors.centerIn: parent
        spacing: 24
        Row {
            spacing: 16
            MD.Button {
                text: "Fixed"
                mdState.type: MD.Enum.BtFilled
            }
            MD.Button {
                text: "Interactive"
                mdState.type: MD.Enum.BtFilled
                mdState.shapeMode: MD.Enum.InteractiveShape
            }
        }
        Row {
            spacing: 16
            MD.ToggleButton {
                text: "Selected"
                checked: true
            }
            MD.IconToggleButton {
                icon.name: MD.Token.icon.star
                checked: true
                mdState.type: MD.Enum.IBtFilledTonal
                mdState.shapeMode: MD.Enum.InteractiveShape
            }
            MD.IconButton {
                icon.name: MD.Token.icon.favorite
                mdState.type: MD.Enum.IBtFilled
                mdState.widthMode: MD.Enum.NarrowWidth
                mdState.shapeMode: MD.Enum.InteractiveShape
            }
        }
        MD.SplitButton {
            text: "Split"
        }
    }
}
