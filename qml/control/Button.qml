import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property MD.StateButton mdState: MD.StateButton {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: mdState.type == MD.Enum.BtText || mdState.type == MD.Enum.BtOutlined
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    // https://m3.material.io/components/buttons/specs
    leftPadding: mdState.leadingSpace
    rightPadding: mdState.trailingSpace
    spacing: mdState.spacing

    icon.width: mdState.iconSize
    icon.height: mdState.iconSize

    property MD.typescale typescale: MD.Token.typescale.label_large
    font.capitalization: Font.MixedCase // M3 uses mixed case for buttons
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: MD.IconLabel {
        text: control.text
        color: control.mdState.textColor
        style: control.iconStyle
        icon.name: control.icon.name
        icon.size: control.icon.width
        opacity: control.mdState.contentOpacity

        label.lineHeight: control.typescale.line_height
    }

    background: MD.ElevationRectangle {
        implicitWidth: 64
        implicitHeight: control.mdState.containerHeight

        corners: control.mdState.corners
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity


        border.width: control.mdState.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: control.enabled ? control.mdState.ctx.color.outline : control.mdState.ctx.color.on_surface
        elevation: control.mdState.elevation
        elevationVisible: (control.mdState.type == MD.Enum.BtElevated) && !control.flat && color.a > 0

        MD.Ripple2 {
            anchors.fill: parent
            corners: parent.corners
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }
}
