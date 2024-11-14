import QtQuick
import Qcm.Material as MD

MD.State {
    id: root

    elevation: MD.Token.elevation.level0
    textColor: control.checked ? root.ctx.color.on_surface : root.ctx.color.on_surface_variant
    backgroundColor: control.checked ? root.ctx.color.secondary_container : "transparent"
    supportTextColor: control.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"

    states: [
        State {
            name: "Pressed"
            when: control.down || control.visualFocus
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: control.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: control.hovered
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: control.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
