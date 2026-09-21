pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property MD.StateIconButton mdState: MD.StateIconButton {
        item: control
    }

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
    icon.color: control.mdState.textColor
    icon.fill: control.checked

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        opacity: control.mdState.contentOpacity

        MD.IconView {
            anchors.centerIn: parent
            icon: control.icon
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: control.mdState.containerWidth
        implicitHeight: control.mdState.containerHeight

        corners: control.mdState.corners
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

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
