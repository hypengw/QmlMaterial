import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int type: MD.Enum.BtElevated
    property int size: MD.Enum.S
    property bool isRound: true
   

    elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
    property real corner: calcRadius(root.size, root.isRound, false)
    property MD.corners corners: MD.Util.corners(root.corner)

    function calcRadius(s, round, pressed) {
        if (pressed) {
            switch (s) {
                case MD.Enum.XS: return 8;
                case MD.Enum.S: return 8;
                case MD.Enum.M: return 12;
                case MD.Enum.L: return 16;
                case MD.Enum.XL: return 16;
            }
        }
        if (round) {
            return item.background ? item.background.height / 2 : 20;
        }
        // Square button
        switch (s) {
            case MD.Enum.XS: return 12;
            case MD.Enum.S: return 12;
            case MD.Enum.M: return 16;
            case MD.Enum.L: return 28;
            case MD.Enum.XL: return 28;
        }
        return 12;
    }

    textColor: {
        switch (root.type) {
        case MD.Enum.BtFilled:
            return ctx.color.on_primary;
        case MD.Enum.BtFilledTonal:
            return ctx.color.on_secondary_container;
        case MD.Enum.BtOutlined:
        case MD.Enum.BtText:
        case MD.Enum.BtElevated:
        default:
            return ctx.color.primary;
        }
    }
    backgroundColor: {
        switch (root.type) {
        case MD.Enum.BtFilled:
            return ctx.color.primary;
        case MD.Enum.BtFilledTonal:
            return ctx.color.secondary_container;
        case MD.Enum.BtOutlined:
        case MD.Enum.BtText:
            return "transparent";
        case MD.Enum.BtElevated:
        default:
            return ctx.color.surface_container_low;
        }
    }
    stateLayerColor: "transparent"

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: {
                    switch (root.type) {
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                        return "transparent";
                    case MD.Enum.BtElevated:
                        return root.ctx.color.surface_container_low;
                    default:
                        return root.ctx.color.on_surface;
                    }
                }
                root.contentOpacity: MD.Token.state.disabled_content
                root.backgroundOpacity: {
                    switch (root.type) {
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                        return 1.0;
                    case MD.Enum.BtElevated:
                        return MD.Token.state.disabled_content; // Elevated button container opacity when disabled
                    default:
                        return MD.Token.state.disabled_container;
                    }
                }
            }
        },
        State {
            name: "Pressed"
            when: root.item.down
            PropertyChanges {
                root.elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
                root.corner: calcRadius(root.size, root.isRound, true)
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.BtFilled:
                    case MD.Enum.BtFilledTonal:
                        c = root.ctx.color.getOn(root.backgroundColor);
                        break;
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                    case MD.Enum.BtElevated:
                    default:
                        c = root.ctx.color.primary;
                    }
                    return c;
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level2 : MD.Token.elevation.level1
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.BtFilled:
                    case MD.Enum.BtFilledTonal:
                        c = root.ctx.color.getOn(root.backgroundColor);
                        break;
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                    case MD.Enum.BtElevated:
                    default:
                        c = root.ctx.color.primary;
                    }
                    return c;
                }
            }
        },
        State {
            name: "Focus"
            when: root.item.visualFocus
            PropertyChanges {
                root.elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
                root.stateLayerOpacity: MD.Token.state.focus.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.BtFilled:
                    case MD.Enum.BtFilledTonal:
                        c = root.ctx.color.getOn(root.backgroundColor);
                        break;
                    case MD.Enum.BtOutlined:
                    case MD.Enum.BtText:
                    case MD.Enum.BtElevated:
                    default:
                        c = root.ctx.color.primary;
                    }
                    return c;
                }
            }
        }
    ]

    transitions: [
        Transition {
            from: "Pressed"; to: "Hovered"
            NumberAnimation { properties: "elevation,corner"; duration: MD.Token.duration.short2; easing: MD.Token.easing.standard }
            ColorAnimation { duration: MD.Token.duration.short2 }
            NumberAnimation { property: "stateLayerOpacity"; duration: MD.Token.duration.short2 }
        },
        Transition {
            from: "*"; to: "Pressed"
            NumberAnimation { properties: "elevation,corner"; duration: MD.Token.duration.short1; easing: MD.Token.easing.standard }
            ColorAnimation { duration: MD.Token.duration.short1 }
            NumberAnimation { property: "stateLayerOpacity"; duration: MD.Token.duration.short1 }
        },
        Transition {
            from: "*"; to: "*"
            NumberAnimation { properties: "elevation,corner"; duration: MD.Token.duration.short4; easing: MD.Token.easing.standard }
            ColorAnimation { duration: MD.Token.duration.short4 }
            NumberAnimation { property: "stateLayerOpacity"; duration: MD.Token.duration.short4 }
            NumberAnimation { property: "contentOpacity"; duration: MD.Token.duration.short4 }
            NumberAnimation { property: "backgroundOpacity"; duration: MD.Token.duration.short4 }
        }
    ]
}
