import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.State {
    id: root
    required property T.MenuItem item
    ctx: item.MD.MProp

    elevation: MD.Token.elevation.level2
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface_container
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"
    property color leadingIconColor: root.ctx.color.on_surface
    property color trailingIconColor: root.ctx.color.on_surface

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.item.contentItem.opacity: 0.38
            }
        },
        State {
            name: "Pressed"
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.leadingIconColor: root.ctx.color.on_surface_variant
                root.trailingIconColor: root.ctx.color.on_surface_variant
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.on_surface
            }
        }
    ]
}
