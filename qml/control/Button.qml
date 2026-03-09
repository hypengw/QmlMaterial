import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.BtElevated
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property MD.StateButton mdState: MD.StateButton {
        item: control
    }
    Binding {
        control.mdState.type: control.type
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: type == MD.Enum.BtText || type == MD.Enum.BtOutlined
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    // https://m3.material.io/components/buttons/specs
    leftPadding: {
        if (type == MD.Enum.BtText) {
            return hasIcon ? 12 : 12;
        }
        return hasIcon ? 16 : 24;
    }
    rightPadding: {
        if (type == MD.Enum.BtText) {
            return hasIcon ? 16 : 12;
        }
        return 24;
    }
    spacing: 8

    icon.width: 18 // M3 default is 18dp inside buttons
    icon.height: 18

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
        implicitHeight: 40

        radius: height / 2
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: control.enabled ? control.mdState.ctx.color.outline : control.mdState.ctx.color.on_surface
        elevation: control.mdState.elevation
        elevationVisible: (control.type == MD.Enum.BtElevated) && !control.flat && color.a > 0

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }
}
