import QtQuick
import Qcm.Material as MD

Item {
    id: root

    property real elevation: 0
    property color color: MD.MProp.color.shadow
    property MD.corners corners: MD.Util.corners(0)

    visible: !MD.Util.epsilonEqual(elevation, MD.Token.elevation.level0)

    Behavior on elevation {
        NumberAnimation {
            duration: MD.Token.duration.medium1
            easing: MD.Token.easing.emphasized_accelerate
        }
    }

    // Uniform 4-corner radius → analytic 9-patch path (RRectShadowImpl).
    // Non-uniform radius → BlurMask path, which accepts per-corner radii.
    readonly property bool _uniform_radius: corners.isUniform()

    Loader {
        anchors.fill: parent
        sourceComponent: root._uniform_radius ? analytic_comp : blur_comp

        Component {
            id: analytic_comp
            MD.RRectShadowImpl {
                elevation: root.elevation
                color: root.color
                corners: root.corners
            }
        }

        Component {
            id: blur_comp
            MD.BlurMaskImpl {
                sigma: Math.max(root.elevation * 1.2, 0.5)
                color: Qt.rgba(root.color.r, root.color.g, root.color.b,
                               root.color.a * 0.35)
                corners: root.corners
                style: MD.BlurMaskImpl.Outer
            }
        }
    }
}
