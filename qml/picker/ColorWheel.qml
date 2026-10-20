pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.ColorWheelBase {
    id: root

    readonly property real _cx: width / 2
    readonly property real _cy: height / 2

    implicitWidth: 240
    implicitHeight: 240

    // Hue ring as an annulus filled with conical gradient.
    MD.Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillRule: ShapePath.OddEvenFill
            startX: root._cx + root.outerRadius
            startY: root._cy

            PathAngleArc {
                centerX: root._cx
                centerY: root._cy
                radiusX: root.outerRadius
                radiusY: root.outerRadius
                startAngle: 0
                sweepAngle: 360
            }
            PathMove {
                x: root._cx + root.innerRadius
                y: root._cy
            }
            PathAngleArc {
                centerX: root._cx
                centerY: root._cy
                radiusX: root.innerRadius
                radiusY: root.innerRadius
                startAngle: 0
                sweepAngle: 360
            }

            fillGradient: ConicalGradient {
                centerX: root._cx
                centerY: root._cy
                angle: 0
                GradientStop {
                    position: 0.0000
                    color: "#ff0000"
                }
                GradientStop {
                    position: 0.1667
                    color: "#ffff00"
                }
                GradientStop {
                    position: 0.3333
                    color: "#00ff00"
                }
                GradientStop {
                    position: 0.5000
                    color: "#00ffff"
                }
                GradientStop {
                    position: 0.6667
                    color: "#0000ff"
                }
                GradientStop {
                    position: 0.8333
                    color: "#ff00ff"
                }
                GradientStop {
                    position: 1.0000
                    color: "#ff0000"
                }
            }
        }
    }

    // Hue indicator: radial line across the ring (not a circle).
    Item {
        x: root.handlePosition.x - width / 2
        y: root.handlePosition.y - height / 2
        width: root.thickness + 8
        height: 4
        rotation: -root.hue * 360

        Rectangle {
            anchors.fill: parent
            radius: 2
            color: "white"
            border.color: "black"
            border.width: 1
        }
    }

    // Inscribed SV triangle — vertices touch the inner edge of the ring.
    MD.ColorTriangle {
        id: m_tri
        anchors.centerIn: parent
        width: 2 * root.innerRadius
        height: 2 * root.innerRadius
        hue: root.hue
        saturation: root.saturation
        value: root.value
    }

    triangle: m_tri
    Accessible.role: Accessible.Slider
    Accessible.name: qsTr("Hue")
}
