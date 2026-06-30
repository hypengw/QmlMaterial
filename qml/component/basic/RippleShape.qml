pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root
    property real stateOpacity: 0
    property bool pressed: false
    property real pressX: width / 2
    property real pressY: height / 2

    property real _circle_radius: 0

    property alias color: m_back.color
    property alias corners: m_back.corners
    property alias radius: m_back.radius
    readonly property real _maxCorner: Math.min(root.width, root.height) * 0.5
    readonly property MD.corners _shapeCorners: MD.Util.corners(
        root._clampCorner(m_back.corners.topLeft),
        root._clampCorner(m_back.corners.topRight),
        root._clampCorner(m_back.corners.bottomLeft),
        root._clampCorner(m_back.corners.bottomRight))

    function _clampCorner(value) {
        return Math.min(Math.max(value, 0), root._maxCorner);
    }

    clip: false

    MD.Rectangle {
        id: m_back
        anchors.fill: parent
        color: "transparent"
        opacity: root.stateOpacity
    }

    MD.Shape {
        id: m_circle
        anchors.fill: parent
        opacity: 0.12
        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: m_back.color
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
                    position: 0.77
                    color: MD.Util.transparent(root.color, 1.0)
                }
                GradientStop {
                    position: 0.771
                    color: MD.Util.transparent(root.color, 0.0)
                }
                GradientStop {
                    position: 1
                    color: MD.Util.transparent(root.color, 0.0)
                }
            }

            startX: root._shapeCorners.topLeft

            startY: 0

            PathLine {
                x: root.width - root._shapeCorners.topRight
                y: 0
            }
            PathArc {
                relativeX: root._shapeCorners.topRight
                relativeY: root._shapeCorners.topRight
                radiusX: root._shapeCorners.topRight
                radiusY: root._shapeCorners.topRight
            }
            PathLine {
                x: root.width
                y: root.height - root._shapeCorners.bottomRight
            }
            PathArc {
                relativeX: -root._shapeCorners.bottomRight
                relativeY: root._shapeCorners.bottomRight
                radiusX: root._shapeCorners.bottomRight
                radiusY: root._shapeCorners.bottomRight
            }
            PathLine {
                x: root._shapeCorners.bottomLeft
                y: root.height
            }
            PathArc {
                relativeX: -root._shapeCorners.bottomLeft
                relativeY: -root._shapeCorners.bottomLeft
                radiusX: root._shapeCorners.bottomLeft
                radiusY: root._shapeCorners.bottomLeft
            }
            PathLine {
                x: 0
                y: root._shapeCorners.topLeft
            }
            PathArc {
                x: root._shapeCorners.topLeft
                y: 0
                radiusX: root._shapeCorners.topLeft
                radiusY: root._shapeCorners.topLeft
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
                        m_back.opacity = root.stateOpacity;
                        root._circle_radius = root.endRadius / 1.3 / 4;
                        m_circle.opacity = 0.12;
                    }
                }
                ParallelAnimation {
                    NumberAnimation {
                        target: m_back
                        property: 'opacity'
                        to: 0.12
                        duration: 500
                    }
                    NumberAnimation {
                        target: root
                        property: '_circle_radius'
                        duration: 500
                    }
                }
            }
        },
        Transition {
            from: "active"
            to: "normal"
            ParallelAnimation {
                alwaysRunToEnd: true
                NumberAnimation {
                    target: m_back
                    to: root.stateOpacity
                    property: 'opacity'
                    duration: 200
                }
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
        }
    ]

    // for tracking root.stateOpacity, need to be top-level
    SequentialAnimation {
        id: m_fade
        running: false
        onStopped: {
            m_back.opacity = Qt.binding(function () {
                return root.stateOpacity;
            });
            m_circle.opacity = 0;
        }

        NumberAnimation {
            target: m_back
            to: root.stateOpacity
            duration: 100
            property: "opacity"
        }
        NumberAnimation {
            target: m_circle
            to: 0
            duration: 100
            property: "opacity"
        }
    }
}
