import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Rectangle {
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

            startX: root.radius
            startY: 0

            PathLine {
                x: root.width - root.radius
                y: 0
            }
            PathArc {
                relativeX: root.radius
                relativeY: root.radius
                radiusX: root.radius
                radiusY: root.radius
            }
            PathLine {
                x: root.width
                y: root.height - root.radius
            }
            PathArc {
                relativeX: -root.radius
                relativeY: root.radius
                radiusX: root.radius
                radiusY: root.radius
            }
            PathLine {
                x: root.radius
                y: root.height
            }
            PathArc {
                relativeX: -root.radius
                relativeY: -root.radius
                radiusX: root.radius
                radiusY: root.radius
            }
            PathLine {
                x: 0
                y: root.radius
            }
            PathArc {
                x: root.radius
                y: 0
                radiusX: root.radius
                radiusY: root.radius
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
