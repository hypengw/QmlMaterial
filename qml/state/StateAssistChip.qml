import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Button item
   

    property bool elevated: false
    property color iconColor: ctx.color.primary
    elevation: elevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
    textColor: ctx.color.on_surface
    outlineColor: ctx.color.outline_variant
    backgroundColor: elevated ? ctx.color.surface_container_low : 'transparent'

    state: MD.Util.stateText(item.enabled, item.down, item.hovered, item.visualFocus)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.iconColor: root.ctx.color.on_surface
                root.textColor: root.ctx.color.on_surface
                root.outlineColor: ctx.color.on_surface
                root.contentOpacity: 0.38
                root.backgroundOpacity: 0.12
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
                root.textColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.elevation: root.elevated ? MD.Token.elevation.level2 : MD.Token.elevation.level0
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
                root.textColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
                root.textColor: root.ctx.color.on_surface
                root.outlineColor: root.ctx.color.on_surface
            }
        }
    ]
}
