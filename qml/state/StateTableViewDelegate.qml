import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.TableViewDelegate item

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerOpacity: 0.0
    stateLayerColor: root.ctx.color.on_surface

    state: MD.Util.stateText(item.enabled, item.pressed || item.visualFocus, item.rowHovered, false)

    states: [
        State {
            name: "disabled"
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
