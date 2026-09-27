pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property MD.StateIconButton mdState: MD.StateIconButton {
        item: control
    }

    readonly property MD.ButtonMotion _motion: MD.ButtonMotion {
        source: control.mdState
    }
    Component.onCompleted: _motion.enabled = true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: mdState.type == MD.Enum.IBtStandard || (mdState.type == MD.Enum.IBtOutlined && !control.checked)
    topInset: 4
    bottomInset: 4
    leftInset: 4
    rightInset: 4

    padding: 8
    spacing: 0

    icon.width: mdState.iconSize
    icon.height: mdState.iconSize
    icon.color: control._motion.textColor
    icon.fill: control.checked

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        opacity: control._motion.contentOpacity

        MD.IconView {
            anchors.centerIn: parent
            icon: control.icon
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: control.mdState.containerWidth
        implicitHeight: control.mdState.containerHeight

        corners: control._motion.corners
        color: control._motion.backgroundColor
        opacity: control._motion.backgroundOpacity

        border.width: mdState.type == MD.Enum.IBtOutlined ? 1 : 0
        border.color: control.mdState.ctx.color.outline

        elevationVisible: elevation && color.a > 0 && !control.flat
        elevation: control.mdState.elevation

        MD.Ripple {
            anchors.fill: parent
            corners: parent.corners
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }

        MD.FocusIndicator {
            corners: parent.corners
            active: control.visualFocus
        }
    }
}
