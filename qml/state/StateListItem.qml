import QtQuick
import Qcm.Material as MD

MD.State {
    id: root

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerOpacity: 0.0
    stateLayerColor: root.ctx.color.on_surface

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.38
            }
        },
        State {
            name: "Pressed"
            when: root.item.pressed || root.item.visualFocus
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
