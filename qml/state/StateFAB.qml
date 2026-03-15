import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int type: MD.Enum.FABNormal
    property int color: MD.Enum.FABColorPrimary

    elevation: MD.Token.elevation.level3
    property int corner: {
        switch (type) {
        case MD.Enum.FABSmall:
            return MD.Token.shape.corner.extra_small;
        case MD.Enum.FABLarge:
            return MD.Token.shape.corner.extra_large;
        case MD.Enum.FABNormal:
        default:
            return MD.Token.shape.corner.large;
        }
    }
    textColor: {
        switch (root.color) {
        case MD.Enum.FABColorSurface:
            return root.ctx.color.primary;
        case MD.Enum.FABColorSecondary:
            return root.ctx.color.on_secondary_container;
        case MD.Enum.FABColorTertiary:
            return root.ctx.color.on_tertiary_container;
        case MD.Enum.FABColorPrimary:
        default:
            return root.ctx.color.on_primary_container;
        }
    }
    backgroundColor: {
        switch (root.color) {
        case MD.Enum.FABColorSurface:
            return root.ctx.color.surface_container_high;
        case MD.Enum.FABColorSecondary:
            return root.ctx.color.secondary_container;
        case MD.Enum.FABColorTertiary:
            return root.ctx.color.tertiary_container;
        case MD.Enum.FABColorPrimary:
        default:
            return root.ctx.color.primary_container;
        }
    }
    stateLayerColor: textColor

    states: [
        State {
            name: "Pressed"
            when: root.item.pressed || root.item.visualFocus
            PropertyChanges {
                root.elevation: MD.Token.elevation.level3
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: MD.Token.elevation.level4
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
            }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                properties: "elevation,corner"
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
            NumberAnimation {
                property: "stateLayerOpacity"
                duration: MD.Token.duration.short2
            }
        }
    ]
}
