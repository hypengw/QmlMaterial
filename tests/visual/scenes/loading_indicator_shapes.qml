import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 480
    height: 280
    color: "#101218"

    readonly property var labels: ["SOFT_BURST", "COOKIE_9", "PENTAGON", "PILL", "SUNNY", "COOKIE_4", "OVAL"]
    readonly property int columns: 4
    readonly property int cellSize: 110
    readonly property int cellPad: 8
    readonly property int contentSize: 84

    Repeater {
        model: root.labels.length

        delegate: Item {
            required property int index
            readonly property int col: index % root.columns
            readonly property int row: Math.floor(index / root.columns)

            x: root.cellPad + col * (root.cellSize + root.cellPad)
            y: root.cellPad + row * (root.cellSize + root.cellPad + 16) + 16
            width: root.cellSize
            height: root.cellSize

            Rectangle {
                anchors.fill: parent
                color: "#1c1f27"
                radius: 8
                border.color: "#33384a"
                border.width: 1
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: -16
                text: root.labels[index]
                color: "#cfd2e0"
                font.pixelSize: 11
                font.family: "monospace"
            }

            MD.LoadingIndicatorUpdator {
                id: updator
                progress: index
                colors: ["#80a9ff"]
            }

            MD.Shape {
                anchors.centerIn: parent
                width: root.contentSize
                height: root.contentSize
                MD.MaterialShapePath {
                    shape: updator.shape
                    size: Qt.size(root.contentSize, root.contentSize)
                    radialNormalization: true
                    fillColor: updator.color
                    strokeWidth: 0
                }
            }
        }
    }

    MD.BusyIndicator {
        x: root.cellPad + 3 * (root.cellSize + root.cellPad)
        y: root.cellPad + root.cellSize + root.cellPad + 32
        width: root.cellSize
        height: root.cellSize
        indicatorSize: root.contentSize
        colors: ["#80a9ff"]
        running: true
    }
}
