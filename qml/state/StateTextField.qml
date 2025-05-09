import QtQuick
import Qcm.Material as MD
import QtQuick.Templates as T

MD.State {
    id: root
    // required property T.TextField item

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: {
        switch (item.type) {
        case MD.Enum.TextFieldFilled:
            return ctx.color.surface_container_highest;
        default:
            return "transparent";
        }
    }
    supportTextColor: root.ctx.color.on_surface_variant
    outlineColor: root.ctx.color.outline
    property color placeholderColor: ctx.color.on_surface_variant

    property MD.t_typescale typescale: {
        switch (item.type) {
        case MD.Enum.TextFieldFilled:
            return MD.Token.typescale.body_large;
        default:
            return MD.Token.typescale.title_large;
        }
    }
    property int containerHeight: item.type === MD.Enum.TextFieldFilled ? 56 : 64
    property int corner: MD.Token.shape.corner.extra_small

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.placeholderColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.on_surface
                placeholder.opacity: 0.38
                //          root.item.background.opacity: 0.12
            }
        },
        State {
            name: "ErrorHover"
            when: !root.item.acceptableInput && root.item.hovered
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.placeholderColor: root.ctx.color.on_error_container
                root.outlineColor: root.ctx.color.on_error_container
            }
        },
        State {
            name: "Error"
            when: !root.item.acceptableInput
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.placeholderColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.error
            }
        },
        State {
            name: "Focused"
            when: root.item.focus
            PropertyChanges {
                root.placeholderColor: root.ctx.color.primary
                root.outlineColor: root.ctx.color.primary
            }
            PropertyChanges {
                restoreEntryValues: false
                root.stateLayerColor: {
                    const c = root.ctx.color.primary;
                    return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.placeholderColor: root.ctx.color.on_surface
                root.outlineColor: root.ctx.color.on_surface
            }
            PropertyChanges {
                restoreEntryValues: false
                root.stateLayerColor: {
                    const c = root.ctx.color.primary;
                    return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                }
            }
        }
    ]
}
