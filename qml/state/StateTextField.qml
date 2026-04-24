import QtQuick
import Qcm.Material as MD
import QtQuick.Templates as T

MD.MState {
    id: root
    required property T.TextField item
    property int type


    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: {
        switch (root.type) {
        case MD.Enum.TextFieldFilled:
            return ctx.color.surface_container_highest;
        default:
            return "transparent";
        }
    }
    supportTextColor: root.ctx.color.on_surface_variant
    outlineColor: root.ctx.color.outline
    property int indicatorHeight: 1
    property color indicatorColor: root.ctx.color.on_surface_variant
    property color placeholderColor: root.ctx.color.on_surface_variant

    property MD.typescale typescale: {
        switch (root.type) {
        case MD.Enum.TextFieldFilled:
            return MD.Token.typescale.body_large;
        default:
            return MD.Token.typescale.title_large;
        }
    }
    property int containerHeight: root.type === MD.Enum.TextFieldFilled ? 56 : 64
    property int corner: MD.Token.shape.corner.extra_small

    state: {
        if (!item.enabled) return "disabled";
        if (!item.acceptableInput && item.hovered) return "errorHover";
        if (!item.acceptableInput) return "error";
        if (item.focus) return "focus";
        if (item.hovered) return "hovered";
        return "";
    }

    readonly property list<State> filledStates: [
        State {
            name: "disabled"
            PropertyChanges {
                root.placeholderColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.on_surface
                placeholder.opacity: MD.Token.state.disabled_content
                //          root.item.background.opacity: MD.Token.state.disabled_container
            }
        },
        State {
            name: "errorHover"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.placeholderColor: root.ctx.color.on_error_container
                root.outlineColor: root.ctx.color.on_error_container
            }
        },
        State {
            name: "error"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.placeholderColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.error
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                root.placeholderColor: root.ctx.color.primary
                root.outlineColor: root.ctx.color.primary
                root.indicatorColor: root.ctx.color.primary
                root.indicatorHeight: 2
                root.stateLayerColor: root.ctx.color.primary
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.placeholderColor: root.ctx.color.on_surface
                root.outlineColor: root.ctx.color.on_surface
                root.indicatorColor: root.ctx.color.on_surface
                root.stateLayerColor: root.ctx.color.primary
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]

    readonly property list<State> outlineStates: [
        State {
            name: "disabled"
            PropertyChanges {
                root.placeholderColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.on_surface
                placeholder.opacity: MD.Token.state.disabled_content
                //          root.item.background.opacity: MD.Token.state.disabled_container
            }
        },
        State {
            name: "errorHover"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.placeholderColor: root.ctx.color.on_error_container
                root.outlineColor: root.ctx.color.on_error_container
            }
        },
        State {
            name: "error"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.placeholderColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.error
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                root.placeholderColor: root.ctx.color.primary
                root.outlineColor: root.ctx.color.primary
                root.indicatorColor: root.ctx.color.primary
                root.indicatorHeight: 2
                root.stateLayerColor: root.ctx.color.primary
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.placeholderColor: root.ctx.color.on_surface
                root.outlineColor: root.ctx.color.on_surface
                root.indicatorColor: root.ctx.color.on_surface
                root.stateLayerColor: root.ctx.color.primary
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]

    states: root.type == MD.Enum.TextFieldFilled ? filledStates : outlineStates
}
