import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Basic.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.BtElevated
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property alias mdState: m_sh.state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: type == MD.Enum.BtText || type == MD.Enum.BtOutlined
    topInset: 6
    bottomInset: 6
    verticalPadding: 14
    // https://m3.material.io/components/buttons/specs#256326ad-f934-40e7-b05f-0bcb41aa4382
    leftPadding: flat ? 12 : (hasIcon ? 16 : 24)
    rightPadding: flat ? (hasIcon ? 16 : 12) : 24
    spacing: 8

    icon.width: 24
    icon.height: 24
    font.capitalization: Font.Capitalize

    contentItem: MD.IconLabel {
        font: control.font
        text: control.text
        typescale: MD.Token.typescale.label_large
        color: control.mdState.textColor
        icon_style: control.iconStyle

        icon_name: control.icon.name
        icon_size: control.icon.width
        opacity: control.mdState.contentOpacity
    }

    background: MD.ElevationRectangleBorder {
        implicitWidth: 64
        implicitHeight: 40

        radius: height / 2
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: control.mdState.ctx.color.outline
        elevation: control.mdState.elevation
        elevationVisible: elevation != MD.Token.elevation.level0 && !control.flat && color.a > 0

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

    MD.StateHolder {
        id: m_sh
        state: MD.StateButton {
            item: control
        }
    }
}
