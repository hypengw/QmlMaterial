import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 980
    height: 900
    color: "#fafafa"

    readonly property var names: ["Circle", "Square", "Slanted", "Arch", "Fan", "Arrow", "SemiCircle", "Oval", "Pill", "Triangle", "Diamond", "ClamShell", "Pentagon", "Gem", "Sunny", "VerySunny", "Cookie4Sided", "Cookie6Sided", "Cookie7Sided", "Cookie9Sided", "Cookie12Sided", "Ghostish", "Clover4Leaf", "Clover8Leaf", "Burst", "SoftBurst", "Boom", "SoftBoom", "Flower", "Puffy", "PuffyDiamond", "PixelCircle", "PixelTriangle", "Bun", "Heart"]
    readonly property var colors: ["#227568", "#bf4157", "#526bb4"]

    Repeater {
        model: root.names
        Item {
            required property int index
            required property string modelData
            x: (index % 7) * 140
            y: Math.floor(index / 7) * 124 + 12
            width: 140
            height: 124
            MD.Shape {
                x: 30
                width: 80
                height: 80
                MD.MaterialShapePath {
                    shape: index
                    size: Qt.size(80, 80)
                    fillColor: root.colors[index % 3]
                    strokeWidth: 0
                }
            }
            Text {
                y: 90
                width: parent.width
                text: modelData
                horizontalAlignment: Text.AlignHCenter
                color: "#242628"
                font.pixelSize: 12
            }
        }
    }
    Repeater {
        model: 7
        MD.Shape {
            required property int index
            x: 30 + index * 140
            y: 654
            width: 80
            height: 80
            MD.MaterialShapePath {
                shape: MD.MaterialShape.Heart
                toShape: MD.MaterialShape.Cookie9Sided
                progress: index / 6
                size: Qt.size(80, 80)
                fillColor: "#bf4157"
                strokeWidth: 0
            }
        }
    }
    Repeater {
        model: 7
        Item {
            required property int index
            x: 30 + index * 140
            y: 782
            width: 80
            height: 80
            MD.LoadingIndicatorUpdator {
                id: updater
                progress: index
            }
            MD.Shape {
                anchors.fill: parent
                MD.MaterialShapePath {
                    shape: updater.shape
                    size: Qt.size(80, 80)
                    radialNormalization: true
                    fillColor: "#227568"
                    strokeWidth: 0
                }
            }
        }
    }
}
