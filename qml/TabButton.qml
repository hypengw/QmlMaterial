import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.TabButton {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    spacing: 8

    leftPadding: 12
    rightPadding: 12

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.title_small.weight
    font.pixelSize: MD.Token.typescale.title_small.size
    property int lineHeight: MD.Token.typescale.title_small.line_height

    contentItem: MD.IconLabel {
        lineHeight: control.lineHeight

        font: control.font
        text: control.text
        icon_style: control.iconStyle

        icon_name: control.icon.name
        icon_size: control.icon.width
    }

    background: MD.Ripple2 {
        implicitHeight: 48

        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: item_state.stateLayerOpacity
        color: item_state.stateLayerColor
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level0
        textColor: control.checked ? item_state.ctx.color.on_surface : item_state.ctx.color.on_surface_variant
        backgroundColor: item_state.ctx.color.surface
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.backgroundColor: item_state.ctx.color.on_surface
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Pressed"
                when: control.down || control.visualFocus
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.stateLayerOpacity:MD.Token.state.pressed.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.stateLayerOpacity:MD.Token.state.hover.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            }
        ]
    }
}
