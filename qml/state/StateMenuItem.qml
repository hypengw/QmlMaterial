import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.MenuItem item
    property bool selected: false
   

    elevation: MD.Token.elevation.level2
    textColor: root.ctx.color.on_surface
    backgroundColor: root.selected ? root.ctx.color.secondary_container : "transparent"
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"
    property color leadingIconColor: root.ctx.color.on_surface
    property color trailingIconColor: root.ctx.color.on_surface

    state: MD.Util.stateText(item.enabled, item.down, item.hovered, item.visualFocus)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.item.contentItem.opacity: 0.38
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
