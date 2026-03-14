import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int type
    property int size: MD.Enum.S
    property bool isRound: true
   

    elevation: MD.Token.elevation.level1
    property real corner: calcRadius(root.size, root.isRound, false)

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

    stateLayerColor: "transparent"

    textColor: {
        switch (root.type) {
        case MD.Enum.IBtOutlined:
            if (root.item.checked)
                return root.ctx.color.inverse_on_surface;
            else
                return root.ctx.color.on_surface_variant;
        case MD.Enum.IBtStandard:
            if (root.item.checked)
                return root.ctx.color.primary;
            else
                return root.ctx.color.on_surface_variant;
        case MD.Enum.IBtFilledTonal:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.on_secondary_container;
            else
                return root.ctx.color.on_surface_variant;
        case MD.Enum.IBtFilled:
        default:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.on_primary;
            else
                return root.ctx.color.primary;
        }
    }
    backgroundColor: {
        switch (root.type) {
        case MD.Enum.IBtStandard:
            return "transparent";
        case MD.Enum.IBtOutlined:
            if (root.item.checked)
                return root.ctx.color.inverse_surface;
            else
                return "transparent";
        case MD.Enum.IBtFilledTonal:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.secondary_container;
            else
                return root.ctx.color.surface_container_highest;
        case MD.Enum.IBtFilled:
        default:
            if (!root.item.checkable || root.item.checked)
                return root.ctx.color.primary;
            else
                return root.ctx.color.surface_container_highest;
        }
    }
    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: {
                    switch (root.type) {
                    case MD.Enum.IBtOutlined:
                    case MD.Enum.IBtStandard:
                        return "transparent";
                    default:
                        return root.ctx.color.on_surface;
                    }
                }
                root.contentOpacity: MD.Token.state.disabled_content
                root.backgroundOpacity: MD.Token.state.disabled_container
            }
        },
        State {
            name: "Pressed"
            when: root.item.down || root.item.visualFocus
            PropertyChanges {
                root.elevation: MD.Token.elevation.level1
                root.corner: calcRadius(root.size, root.isRound, true)
                root.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.IBtFilled:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_primary;
                        else
                            c = root.ctx.color.primary;
                        break;
                    case MD.Enum.IBtFilledTonal:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_secondary_container;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    case MD.Enum.IBtOutlined:
                        if (root.item.checked)
                            c = root.ctx.color.inverse_on_surface;
                        else
                            c = root.ctx.color.on_surface;
                        break;
                    case MD.Enum.IBtStandard:
                    default:
                        if (root.item.checked)
                            c = root.ctx.color.primary;
                        else
                            c = root.ctx.color.on_surface_variant;
                    }
                    return c;
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.elevation: MD.Token.elevation.level2
                root.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                root.stateLayerColor: {
                    let c = null;
                    switch (root.type) {
                    case MD.Enum.IBtFilled:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_primary;
                        else
                            c = root.ctx.color.primary;
                        break;
                    case MD.Enum.IBtFilledTonal:
                        if (!root.item.checkable || root.item.checked)
                            c = root.ctx.color.on_secondary_container;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    case MD.Enum.IBtOutlined:
                        if (root.item.checked)
                            c = root.ctx.color.inverse_on_surface;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
                    default:
                    case MD.Enum.IBtStandard:
                        if (root.item.checked)
                            c = root.ctx.color.primary;
                        else
                            c = root.ctx.color.on_surface_variant;
                        break;
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
