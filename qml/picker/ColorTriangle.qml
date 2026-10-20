pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.ColorTriangleBase {
    id: root

    implicitWidth: 180
    implicitHeight: 180

    readonly property color _hueColor: Qt.hsva(hue, 1, 1, 1)

    MD.Shape {
        anchors.fill: parent

        ShapePath {
            fillColor: root._hueColor
            strokeColor: root._hueColor
            strokeWidth: 1
            startX: root.hueVertex.x
            startY: root.hueVertex.y
            PathLine {
                x: root.whiteVertex.x
                y: root.whiteVertex.y
            }
            PathLine {
                x: root.blackVertex.x
                y: root.blackVertex.y
            }
            PathLine {
                x: root.hueVertex.x
                y: root.hueVertex.y
            }
        }
        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            startX: root.hueVertex.x
            startY: root.hueVertex.y
            fillGradient: LinearGradient {
                x1: (root.hueVertex.x + root.blackVertex.x) / 2
                y1: (root.hueVertex.y + root.blackVertex.y) / 2
                x2: root.whiteVertex.x
                y2: root.whiteVertex.y
                GradientStop {
                    position: 0.0
                    color: "#00ffffff"
                }
                GradientStop {
                    position: 1.0
                    color: "#ffffffff"
                }
            }
            PathLine {
                x: root.whiteVertex.x
                y: root.whiteVertex.y
            }
            PathLine {
                x: root.blackVertex.x
                y: root.blackVertex.y
            }
            PathLine {
                x: root.hueVertex.x
                y: root.hueVertex.y
            }
        }
        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            startX: root.hueVertex.x
            startY: root.hueVertex.y
            fillGradient: LinearGradient {
                x1: (root.hueVertex.x + root.whiteVertex.x) / 2
                y1: (root.hueVertex.y + root.whiteVertex.y) / 2
                x2: root.blackVertex.x
                y2: root.blackVertex.y
                GradientStop {
                    position: 0.0
                    color: "#00000000"
                }
                GradientStop {
                    position: 1.0
                    color: "#ff000000"
                }
            }
            PathLine {
                x: root.whiteVertex.x
                y: root.whiteVertex.y
            }
            PathLine {
                x: root.blackVertex.x
                y: root.blackVertex.y
            }
            PathLine {
                x: root.hueVertex.x
                y: root.hueVertex.y
            }
        }
    }

    // SV handle
    Rectangle {
        x: root.handlePosition.x - width / 2
        y: root.handlePosition.y - height / 2
        width: 14
        height: 14
        radius: 7
        color: "transparent"
        border.color: "white"
        border.width: 2
        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: width / 2
            color: "transparent"
            border.color: "black"
            border.width: 1
        }
    }

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Saturation and brightness")
    Accessible.description: Math.round(saturation * 100) + "%, " + Math.round(value * 100) + "%"
}
