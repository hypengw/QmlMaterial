import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 640
    height: 400
    color: "#eeeeee"

    Repeater {
        model: [[8, 30, 8, 30], [30, 8, 30, 8], [0, 30, 8, 16], [30, 30, 30, 30], [8, 8, 8, 8], [4, 24, 16, 0]]
        MD.ElevationRectangle {
            required property int index
            required property var modelData
            x: 40 + (index % 3) * 200
            y: 40 + Math.floor(index / 3) * 120
            width: 160
            height: 60
            color: "#eeeeee"
            corners.topLeft: modelData[0]
            corners.topRight: modelData[1]
            corners.bottomLeft: modelData[2]
            corners.bottomRight: modelData[3]
            elevation: 6
        }
    }

    MD.ButtonGroupContainer {
        x: 100
        y: 320
        variant: MD.ButtonGroupContainer.Connected
        MD.Button {
            text: "One"
            mdState.type: MD.Enum.BtElevated
        }
        MD.Button {
            text: "Two"
            mdState.type: MD.Enum.BtElevated
        }
        MD.Button {
            text: "Three"
            mdState.type: MD.Enum.BtElevated
        }
    }
}
