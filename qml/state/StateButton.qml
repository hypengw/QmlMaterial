import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    property int type: MD.Enum.BtElevated
    property int size: MD.Enum.S
    property bool isRound: true

    readonly property MD.ButtonSizeItem sizeToken: {
        switch (root.size) {
        case MD.Enum.XS:
            return MD.Token.button.xsmall;
        case MD.Enum.S:
            return MD.Token.button.small;
        case MD.Enum.M:
            return MD.Token.button.medium;
        case MD.Enum.L:
            return MD.Token.button.large;
        case MD.Enum.XL:
            return MD.Token.button.xlarge;
        default:
            return MD.Token.button.small;
        }
    }

    elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0

    property real corner: {
        if (root.item.down)
            return sizeToken.pressed_corner_size;
        if (root.isRound)
            return item.background ? item.background.height / 2 : sizeToken.container_height / 2;
        return sizeToken.corner_size;
    }

    property MD.corners corners: MD.Util.corners(root.corner)

    Behavior on corners {
        PropertyAnimation {
            duration: MD.Token.duration.short2
        }
    }

    readonly property real containerHeight: sizeToken.container_height
    readonly property real iconSize: sizeToken.icon_size
    readonly property real leadingSpace: sizeToken.leading_space
    readonly property real trailingSpace: sizeToken.trailing_space
    readonly property real spacing: sizeToken.spacing

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

    state: MD.Util.stateText(item.enabled, item.down, item.hovered, item.visualFocus)

    states: [
        State {
            name: "disabled"
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
            name: "pressed"
            PropertyChanges {
                root.elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
                // Corner logic already handles 'down' via sizeToken
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
            name: "hovered"
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
            name: "focus"
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
            from: "pressed"
            to: "hovered"
            NumberAnimation {
                properties: "elevation,corner"
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
            ColorAnimation {
                duration: MD.Token.duration.short2
            }
            NumberAnimation {
                property: "stateLayerOpacity"
                duration: MD.Token.duration.short2
            }
        },
        Transition {
            from: "*"
            to: "pressed"
            NumberAnimation {
                properties: "elevation,corner"
                duration: MD.Token.duration.short1
                easing: MD.Token.easing.standard
            }
            ColorAnimation {
                duration: MD.Token.duration.short1
            }
            NumberAnimation {
                property: "stateLayerOpacity"
                duration: MD.Token.duration.short1
            }
        },
        Transition {
            from: "*"
            to: "*"
            NumberAnimation {
                properties: "elevation,corner"
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.standard
            }
            ColorAnimation {
                duration: MD.Token.duration.short4
            }
            NumberAnimation {
                property: "stateLayerOpacity"
                duration: MD.Token.duration.short4
            }
            NumberAnimation {
                property: "contentOpacity"
                duration: MD.Token.duration.short4
            }
            NumberAnimation {
                property: "backgroundOpacity"
                duration: MD.Token.duration.short4
            }
        }
    ]
}
