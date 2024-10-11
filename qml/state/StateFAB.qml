import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.State {
    id: item_state
    item: control

    elevation: MD.Token.elevation.level3
    textColor: {
        switch (control.color) {
        case MD.Enum.FABColorSurfaec:
            return item_state.ctx.color.primary;
        case MD.Enum.FABColorSecondary:
            return item_state.ctx.color.on_secondary_container;
        case MD.Enum.FABColorTertiary:
            return item_state.ctx.color.on_tertiary_container;
        case MD.Enum.FABColorPrimary:
        default:
            return item_state.ctx.color.on_primary_container;
        }
    }
    backgroundColor: {
        switch (control.color) {
        case MD.Enum.FABColorSurfaec:
            return item_state.ctx.color.surface_container_high;
        case MD.Enum.FABColorSecondary:
            return item_state.ctx.color.secondary_container;
        case MD.Enum.FABColorTertiary:
            return item_state.ctx.color.tertiary_container;
        case MD.Enum.FABColorPrimary:
        default:
            return item_state.ctx.color.primary_container;
        }
    }
    stateLayerColor: textColor

    states: [
        State {
            name: "Pressed"
            when: control.pressed || control.visualFocus
            PropertyChanges {
                item_state.elevation: MD.Token.elevation.level3
                item_state.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "Hovered"
            when: control.hovered
            PropertyChanges {
                item_state.elevation: MD.Token.elevation.level4
                item_state.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]
}
