import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    elevation: MD.Token.elevation.level0
    textColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface_variant
    backgroundColor: root.item.checked ? ctx.color.secondary_container : "transparent"
    supportTextColor: ctx.color.on_surface_variant
    stateLayerColor: "transparent"

    property T.ItemDelegate item

    state: MD.Util.stateText(item?.enabled ?? true, (item?.pressed ?? false) || (item?.visualFocus ?? false), item?.hovered ?? false, false)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.item.contentItem.opacity: 0.38
                root.item.background.opacity: 0.38
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.textColor: root.item.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: {
                    const c = root.ctx.color.on_secondary_container;
                    return c;
                }
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.textColor: root.item.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: {
                    const c = root.item.checked ? root.ctx.color.on_secondary_container : root.ctx.color.on_surface;
                    return c;
                }
            }
        }
    ]
}
