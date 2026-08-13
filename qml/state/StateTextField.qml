import QtQuick
import Qcm.Material as MD
import QtQuick.Templates as T

MD.MState {
    id: root
    required property T.TextField item
    property int type
    property int size: MD.Enum.M

    readonly property MD.TextFieldSizeItem sizeToken: {
        switch (root.size) {
        case MD.Enum.XS:
            return MD.Token.text_field.xsmall;
        case MD.Enum.S:
            return MD.Token.text_field.small;
        case MD.Enum.M:
            return MD.Token.text_field.medium;
        case MD.Enum.L:
            return MD.Token.text_field.large;
        case MD.Enum.XL:
            return MD.Token.text_field.xlarge;
        default:
            return MD.Token.text_field.medium;
        }
    }

    readonly property real containerHeight: sizeToken.container_height
    readonly property real horizontalPadding: sizeToken.horizontal_padding
    readonly property real verticalPadding: sizeToken.vertical_padding
    readonly property real iconSize: sizeToken.icon_size
    readonly property real spacing: sizeToken.icon_spacing

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
    property real placeholderOpacity: 1.0

    property MD.typescale typescale: sizeToken.type_scale
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
                root.placeholderOpacity: MD.Token.state.disabled_content
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
                root.placeholderOpacity: MD.Token.state.disabled_content
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
