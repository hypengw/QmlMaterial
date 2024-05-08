import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.CardElevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    verticalPadding: 0
    horizontalPadding: 16

    contentItem: Item {}

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 64

        radius: 12
        color: MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.CardOutlined ? 1 : 0
        border.color: item_state.ctx.color.outline

        layer.enabled: control.enabled && color.a > 0 && !control.flat
        layer.effect: MD.RoundedElevationEffect {
            elevation: MD.MatProp.elevation
        }
    }

    Binding {
        control.contentItem.z: -1
        control.background.z: -2
        when: control.contentItem
    }
    MD.Ripple2 {
        anchors.fill: parent
        radius: 12
        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: item_state.stateLayerOpacity
        color: item_state.stateLayerColor
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor

    MD.State {
        id: item_state
        item: control
        elevation: {
            switch (control.type) {
            case MD.Enum.CardOutlined:
            case MD.Enum.CardFilled:
                return MD.Token.elevation.level0;
            case MD.Enum.CardElevated:
            default:
                return MD.Token.elevation.level1;
            }
        }
        textColor: item_state.ctx.color.getOn(backgroundColor)
        backgroundColor: {
            switch (control.type) {
            case MD.Enum.CardOutlined:
                return item_state.ctx.color.surface;
            case MD.Enum.CardFilled:
                return item_state.ctx.color.surface_container_highest;
            case MD.Enum.CardElevated:
            default:
                return item_state.ctx.color.surface_container_low;
            }
        }
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level1;
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level0;
                        }
                    }
                    item_state.backgroundColor: item_state.ctx.color.surface_variant
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Pressed"
                when: control.down || control.visualFocus
                PropertyChanges {
                    item_state.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level0;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level1;
                        }
                    }
                    item_state.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level1;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level2;
                        }
                    }
                    item_state.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            }
        ]
    }
}
