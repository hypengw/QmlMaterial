import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.ComboBox item


    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: "transparent"
    supportTextColor: root.ctx.color.on_surface_variant
    outlineColor: root.ctx.color.outline

    state: {
        if (!item.enabled) return "disabled";
        if (!item.acceptableInput && item.hovered) return "errorHover";
        if (!item.acceptableInput) return "error";
        if (item.visualFocus) return "focus";
        if (item.hovered) return "hovered";
        return "";
    }

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.supportTextColor: root.ctx.color.on_surface
                root.item.contentItem.opacity: 0.38
                root.item.background.opacity: 0.12
            }
        },
        State {
            name: "error"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.error
            }
        },
        State {
            name: "errorHover"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.supportTextColor: root.ctx.color.error
                root.outlineColor: root.ctx.color.on_error_container
            }
        },
        State {
            name: "focus"
            PropertyChanges {
                root.outlineColor: root.ctx.color.primary
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.outlineColor: root.ctx.color.on_surface
            }
        }
    ]
}
