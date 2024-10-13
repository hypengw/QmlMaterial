import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Rectangle {
    id: root
    property real stateOpacity: 0.0
    property bool pressed: false
    property real pressX: width / 2
    property real pressY: height / 2

    color: "transparent"
    clip: false
    opacity: stateOpacity

    Shape {
        id: circle
        anchors.fill: parent
        property real radius: 0.0
        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: root.color
            fillGradient: RadialGradient {
                centerX: root.pressX
                centerY: root.pressY
                centerRadius: circle.radius
                focalX: centerX
                focalY: centerY

                GradientStop {
                    position: 0
                    color: MD.Util.transparent(root.color, 1.0)
                }
                GradientStop {
                    position: 0.7
                    color: MD.Util.transparent(root.color, 0.7)
                }
                GradientStop {
                    position: 1
                    color: MD.Util.transparent(root.color, 0.0)
                }
            }

            startX: root.corners.topLeft
           
            startY: 0

            PathLine {
                x: root.width - root.corners.topRight
                y: 0
            }
            PathArc {
                relativeX: root.corners.topRight
                relativeY: root.corners.topRight
                radiusX: root.corners.topRight
                radiusY: root.corners.topRight
            }
            PathLine {
                x: root.width
                y: root.height - root.corners.bottomRight
            }
            PathArc {
                relativeX: -root.corners.bottomRight
                relativeY: root.corners.bottomRight
                radiusX: root.corners.bottomRight
                radiusY: root.corners.bottomRight
            }
            PathLine {
                x: root.corners.bottomLeft
                y: root.height
            }
            PathArc {
                relativeX: -root.corners.bottomLeft
                relativeY: -root.corners.bottomLeft
                radiusX: root.corners.bottomLeft
                radiusY: root.corners.bottomLeft
            }
            PathLine {
                x: 0
                y: root.corners.topLeft
            }
            PathArc {
                x: root.corners.topLeft
                y: 0
                radiusX: root.corners.topLeft
                radiusY: root.corners.topLeft
            }
        }
    }

    state: "normal"

    property real endRadius: Math.sqrt(root.height * root.height + root.width * root.width) * 1.3

    states: [
        State {
            name: "active"
            when: root.pressed
            PropertyChanges {
                circle.radius: endRadius
            }
        },
        State {
            name: "normal"
            when: true
            PropertyChanges {
                circle.opacity: 0.0
                circle.radius: 0.0
            }
        }
    ]

    transitions: [
        Transition {
            from: "normal"
            to: "active"
            ParallelAnimation {
                NumberAnimation {
                    target: circle
                    property: 'radius'
                    duration: 500
                }
            }
        },
        Transition {
            from: "active"
            to: "normal"
            ParallelAnimation {
                SequentialAnimation {
                    NumberAnimation {
                        target: circle
                        property: 'radius'
                        to: root.endRadius
                        duration: 200
                    }
                    OpacityAnimator {
                        target: circle
                        duration: 200
                    }
                }
            }
        }
    ]
}
