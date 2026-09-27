import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.ButtonBase {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: !icon.empty
    property MD.StateButtonBase mdState: MD.StateButton {
        item: control
    }

    readonly property MD.ButtonMotion _motion: MD.ButtonMotion {
        source: control.mdState
    }
    Component.onCompleted: _motion.enabled = true

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
    MD.ButtonGroupContainer.defaultCompressionLimit: Math.min(leftPadding, rightPadding)
    MD.ButtonGroupContainer.defaultMinimumWidth: implicitContentWidth

    icon.width: mdState.iconSize
    icon.height: mdState.iconSize
    icon.color: control._motion.textColor

    property MD.typescale typescale: MD.Token.typescale.label_large
    font.capitalization: Font.MixedCase // M3 uses mixed case for buttons
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: Lite.Box {
        alignment: Qt.AlignCenter
        opacity: control._motion.contentOpacity

        Lite.Row {
            transform: Translate {
                x: control.MD.ButtonGroupContainer.connected ? control.mdState.groupOpticalOffset(control._motion.corners) : 0
            }
            width: control.MD.ButtonGroupContainer.grouped ? implicitWidth : Math.min(implicitWidth, parent.width)
            height: Math.min(implicitHeight, parent.height)
            alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: control.spacing

            MD.IconView {
                visible: control.iconStyle != MD.Enum.TextOnly && control.hasIcon
                icon: control.icon
            }

            MD.Label {
                visible: control.iconStyle != MD.Enum.IconOnly
                text: control.text
                color: control._motion.textColor
                useTypescale: false
                font: control.font
                lineHeight: control.typescale.line_height
                wrapMode: Text.NoWrap
                Lite.Layout.fillWidth: true
            }
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 64
        implicitHeight: control.mdState.containerHeight

        corners: control._motion.corners
        color: control._motion.backgroundColor
        opacity: control._motion.backgroundOpacity

        border.width: control.mdState.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: control.enabled ? control.mdState.ctx.color.outline : control.mdState.ctx.color.on_surface
        elevation: control.mdState.elevation
        elevationVisible: (control.mdState.type == MD.Enum.BtElevated) && !control.flat && color.a > 0

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
