import QtQuick
import Qcm.Material as MD

MD.MState {
    id: root
    required property MD.SwitchBase item

    elevation: MD.Token.elevation.level1
    textColor: root.item.checked ? root.ctx.color.on_primary_container : root.ctx.color.surface_container_highest
    backgroundColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.surface_container_highest
    stateLayerColor: "transparent"
    property color handleColor: root.item.checked ? root.ctx.color.on_primary : root.ctx.color.outline
    readonly property bool hasIcon: !root.item.icon.empty
    property int handleSize: (root.item.checked || root.hasIcon) ? 24 : 16

    state: MD.Util.stateText(item.enabled, item.down, item.hovered, false)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.contentOpacity: MD.Token.state.disabled_content
                root.backgroundOpacity: MD.Token.state.disabled_container
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.textColor: root.item.checked ? root.ctx.color.on_primary_container : root.ctx.color.surface_container_highest
                root.backgroundColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.surface_container_highest
                root.handleColor: root.item.checked ? root.ctx.color.primary_container : root.ctx.color.on_surface_variant
                root.handleSize: 28
                root.stateLayerColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.textColor: root.item.checked ? root.ctx.color.on_primary_container : root.ctx.color.surface_container_highest
                root.backgroundColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.surface_container_highest
                root.handleColor: root.item.checked ? root.ctx.color.primary_container : root.ctx.color.on_surface_variant
                root.stateLayerColor: root.item.checked ? root.ctx.color.primary : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
