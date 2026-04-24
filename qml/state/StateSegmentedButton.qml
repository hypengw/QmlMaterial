import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int position: MD.Enum.PosSingle
    property real cornerRadius: 20

    property MD.corners corners: {
        switch (root.position) {
        case MD.Enum.PosFirst:
            return MD.Util.corners(root.cornerRadius, 0, root.cornerRadius, 0);
        case MD.Enum.PosMiddle:
            return MD.Util.corners(0);
        case MD.Enum.PosLast:
            return MD.Util.corners(0, root.cornerRadius, 0, root.cornerRadius);
        case MD.Enum.PosSingle:
        default:
            return MD.Util.corners(root.cornerRadius);
        }
    }

    textColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface
    backgroundColor: item.checked ? ctx.color.secondary_container : "transparent"
    outlineColor: ctx.color.outline

    state: MD.Util.stateText(item.enabled, item.down, item.hovered, item.visualFocus)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: root.item.checked ? root.ctx.color.on_surface : "transparent"
                root.contentOpacity: 0.38
                root.backgroundOpacity: root.item.checked ? 0.12 : 1.0
                root.outlineColor: root.ctx.color.on_surface
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: item.checked ? ctx.color.on_secondary_container : ctx.color.on_surface
            }
        }
    ]
}
