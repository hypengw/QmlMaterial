import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.ItemDelegate item

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.textColor
    backgroundColor: root.ctx.backgroundColor
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerColor: root.ctx.color.on_surface
    stateLayerOpacity: 0.0

    state: MD.Util.stateText(item.enabled, item.pressed, item.hovered, false)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.contentOpacity: MD.Token.state.disabled_content
                root.backgroundOpacity: MD.Token.state.disabled_content
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
