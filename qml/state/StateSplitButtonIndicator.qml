import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Button item
    
    property int type: MD.Enum.BtFilled
    property int size: MD.Enum.S
    property bool isRound: true

    elevation: root.type == MD.Enum.BtElevated ? MD.Token.elevation.level1 : MD.Token.elevation.level0
    
    readonly property real baseCorner: calcRadius(root.size, root.isRound, false)
    readonly property real innerCorner: 4

    property MD.corners corners: MD.Util.corners(innerCorner, baseCorner, innerCorner, baseCorner)

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
        case MD.Enum.BtFilled: return ctx.color.on_primary;
        case MD.Enum.BtFilledTonal: return ctx.color.on_secondary_container;
        case MD.Enum.BtOutlined:
        case MD.Enum.BtText:
        case MD.Enum.BtElevated:
        default: return ctx.color.primary;
        }
    }

    backgroundColor: {
        switch (root.type) {
        case MD.Enum.BtFilled: return ctx.color.primary;
        case MD.Enum.BtFilledTonal: return ctx.color.secondary_container;
        case MD.Enum.BtOutlined:
        case MD.Enum.BtText: return "transparent";
        case MD.Enum.BtElevated:
        default: return ctx.color.surface_container_low;
        }
    }

    stateLayerColor: {
        switch (root.type) {
        case MD.Enum.BtFilled:
        case MD.Enum.BtFilledTonal:
            return ctx.color.getOn(backgroundColor);
        default:
            return ctx.color.primary;
        }
    }

    stateLayerOpacity: {
        if (item.pressed) return MD.Token.state.pressed.state_layer_opacity;
        if (item.hovered) return MD.Token.state.hover.state_layer_opacity;
        if (item.visualFocus) return MD.Token.state.focus.state_layer_opacity;
        return 0;
    }

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.elevation: MD.Token.elevation.level0
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: (root.type == MD.Enum.BtFilled || root.type == MD.Enum.BtFilledTonal) ? root.ctx.color.on_surface : "transparent"
                root.contentOpacity: MD.Token.state.disabled_content
                root.backgroundOpacity: MD.Token.state.disabled_container
            }
        }
    ]
}
