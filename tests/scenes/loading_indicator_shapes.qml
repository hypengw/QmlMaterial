import QtQuick
import Qcm.Material as MD

// Visual debug for LoadingIndicator: lays out each of the 7 predefined
// indeterminate shapes (SOFT_BURST → COOKIE_9 → PENTAGON → PILL → SUNNY →
// COOKIE_4 → OVAL) in a 4-column grid, in declaration order, so we can
// visually compare against MDC-Android's INDETERMINATE_SHAPES reference.
//
// Each cell drives a private LoadingIndicatorUpdator with `progress` set to
// the shape's integer index — at integer progress the spring fraction is 0,
// rotation is 0, and currentShape resolves to predefined_shapes[index]
// untouched, so what the user sees here is the raw geometry, not the
// animated morph.
Rectangle {
    id: root
    width: 480
    height: 280
    color: "#101218"

    readonly property var labels: [
        "SOFT_BURST", "COOKIE_9", "PENTAGON", "PILL",
        "SUNNY", "COOKIE_4", "OVAL"
    ]
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

            MD.BusyIndicatorShape {
                anchors.centerIn: parent
                scale: root.contentSize / 2
                color: updator.color
                points: updator.currentShape
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        text: "qml_material::LoadingIndicatorUpdator predefined shapes (progress=i, rotation=0)"
        color: "#7a8094"
        font.pixelSize: 10
        font.family: "monospace"
    }
}
