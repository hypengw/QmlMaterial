import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Control item

    elevation: MD.Token.elevation.level3
    textColor: ctx.color.inverse_primary
    supportTextColor: ctx.color.inverse_on_surface
    backgroundColor: ctx.color.inverse_surface
    stateLayerColor: "transparent"
    property color iconColor: ctx.color.inverse_on_surface
    property color iconStateLayerColor: "transparent"
    property real iconStateLayerOpacity: 0.0

    state: {
        if (item.actionControl.hovered) return "hovered:action";
        if (item.actionControl.visualFocus) return "focus:action";
        if (item.actionControl.down) return "pressed:action";
        if (item.iconControl.hovered) return "hovered:icon";
        if (item.iconControl.visualFocus) return "focus:icon";
        if (item.iconControl.down) return "pressed:icon";
        return "";
    }

    states: [
        State {
            name: "hovered:action"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: root.ctx.color.inverse_primary
                root.textColor: root.ctx.color.inverse_primary
            }
        },
        State {
            name: "focus:action"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: root.ctx.color.inverse_primary
                root.textColor: root.ctx.color.inverse_primary
            }
        },
        State {
            name: "pressed:action"
            PropertyChanges {
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: root.ctx.color.inverse_primary
                root.textColor: root.ctx.color.inverse_primary
            }
        },
        State {
            name: "hovered:icon"
            PropertyChanges {
                root.iconStateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.iconStateLayerColor: root.ctx.color.inverse_on_surface
                root.iconColor: root.ctx.color.inverse_on_surface
            }
        },
        State {
            name: "focus:icon"
            PropertyChanges {
                root.iconStateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.iconStateLayerColor: root.ctx.color.inverse_on_surface
                root.iconColor: root.ctx.color.inverse_on_surface
            }
        },
        State {
            name: "pressed:icon"
            PropertyChanges {
                root.iconStateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.iconStateLayerColor: root.ctx.color.inverse_on_surface
                root.iconColor: root.ctx.color.inverse_on_surface
            }
        }
    ]
}
