pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ElevationRectangle {
    id: root
    required property var button
    required property var mdState
    required property MD.ButtonMotion motion
    property bool outlined: false
    property bool flat: false
    corners: motion.corners
    color: motion.backgroundColor
    opacity: motion.backgroundOpacity
    border.width: outlined ? 1 : 0
    border.color: mdState.ctx.color.outline
    elevation: mdState.elevation
    elevationVisible: elevation > 0 && !flat && color.a > 0
    MD.Ripple {
        anchors.fill: parent
        corners: parent.corners
        interactionSource: root.button.interactionSource
        indication: root.mdState.indication
        animationsEnabled: root.button.animationsEnabled
        color: root.mdState.stateLayerColor
    }
    MD.FocusIndicator {
        corners: parent.corners
        active: root.button.focusVisible
    }
}
