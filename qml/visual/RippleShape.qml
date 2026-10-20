pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root
    property real stateOpacity: 0

    readonly property real _circle_radius: Math.hypot(width, height) * motion.progress * 1.3

    property color color: "transparent"
    property MD.InteractionSource interactionSource: null
    property alias indication: motion.indication
    property real pressOpacity: MD.Token.state.pressed.state_layer_opacity
    property bool animationsEnabled: true
    property alias corners: m_back.corners
    property alias radius: m_back.radius
    readonly property real _maxCorner: Math.min(root.width, root.height) * 0.5
    readonly property MD.corners _shapeCorners: MD.Util.corners(root._clampCorner(m_back.corners.topLeft), root._clampCorner(m_back.corners.topRight), root._clampCorner(m_back.corners.bottomLeft), root._clampCorner(m_back.corners.bottomRight))

    function _clampCorner(value) {
        return Math.min(Math.max(value, 0), root._maxCorner);
    }

    clip: false

    MD.RippleMotion {
        id: motion
        target: root
        interactionSource: root.interactionSource
        stateOpacity: root.stateOpacity
        pressOpacity: root.pressOpacity
        color: root.color
        enabled: root.animationsEnabled
    }

    MD.Shape {
        id: m_circle
        anchors.fill: parent
        opacity: motion.rippleOpacity
        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillColor: m_back.color
            fillGradient: RadialGradient {
                centerX: motion.origin.x
                centerY: motion.origin.y
                centerRadius: root._circle_radius
                focalX: centerX
                focalY: centerY

                GradientStop {
                    position: 0
                    color: MD.Util.transparent(motion.ink, 1.0)
                }
                GradientStop {
                    position: 0.77
                    color: MD.Util.transparent(motion.ink, 1.0)
                }
                GradientStop {
                    position: 0.771
                    color: MD.Util.transparent(motion.ink, 0.0)
                }
                GradientStop {
                    position: 1
                    color: MD.Util.transparent(motion.ink, 0.0)
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

    MD.Rectangle {
        id: m_back
        anchors.fill: parent
        color: motion.ink
        opacity: motion.opacity
    }
}
