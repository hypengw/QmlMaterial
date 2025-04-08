pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Rectangle {
    id: root
    property real stateOpacity: 0
    property bool pressed: false
    property real pressX: width / 2
    property real pressY: height / 2

    property real _circle_radius: 0

    color: "transparent"
    clip: false
    opacity: stateOpacity

    MD.Shape {
        id: m_circle
        anchors.fill: parent
        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: root.color
            fillGradient: RadialGradient {
                centerX: root.pressX
                centerY: root.pressY
                centerRadius: root._circle_radius
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

    readonly property real endRadius: Math.sqrt(root.height * root.height + root.width * root.width) * 1.3

    states: [
        State {
            name: "active"
            when: root.pressed
            PropertyChanges {
                restoreEntryValues: false
                root._circle_radius: endRadius
            }
        },
        State {
            name: "normal"
            when: true
        }
    ]

    transitions: [
        Transition {
            from: "normal"
            to: "active"
            SequentialAnimation {
                ScriptAction {
                    script: {
                        m_fade.stop();
                        root.opacity = root.stateOpacity;
                        root._circle_radius = 0.0;
                        m_circle.opacity = 1.0;
                    }
                }
                NumberAnimation {
                    target: root
                    property: '_circle_radius'
                    duration: 500
                }
            }
        },
        Transition {
            from: "active"
            to: "normal"
            SequentialAnimation {
                NumberAnimation {
                    target: root
                    property: '_circle_radius'
                    to: root.endRadius
                    duration: 200
                }

                ScriptAction {
                    script: {
                        m_fade.start();
                    }
                }
            }
        }
    ]

    // for tracking root.stateOpacity, need to be top-level
    SequentialAnimation {
        id: m_fade
        running: false
        onFinished: {
            root.opacity = Qt.binding(function () {
                return root.stateOpacity;
            });
        }

        OpacityAnimator {
            target: root
            to: root.stateOpacity
            duration: 200
        }
        OpacityAnimator {
            target: m_circle
            to: 0
            duration: 100
        }
    }
}
