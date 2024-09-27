import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

import "js/jsUtils.mjs" as UT

T.Button {
    id: control

    property int type: MD.Enum.FABNormal
    property int color: MD.Enum.FABColorPrimary
    property QtObject flickable: null
    property alias mdState: item_state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: 16
    anchors.bottomMargin: 16

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: _size(type, 8, 16, 30)
    spacing: 0

    icon.width: _size(type, 24, 24, 36)
    icon.height: _size(type, 24, 24, 36)

    font.weight: MD.Token.typescale.label_large.weight
    font.pixelSize: Math.min(icon.width, icon.height)
    font.family: MD.Token.font.icon_round.family

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        Text {
            anchors.centerIn: parent
            font: control.font
            text: control.icon.name
            color: control.mdState.textColor
            lineHeight: font.pixelSize
            lineHeightMode: Text.FixedHeight
        }
    }

    background: Rectangle {
        implicitWidth: control._size(control.type, 40, 56, 96)
        implicitHeight: control._size(control.type, 40, 56, 96)

        radius: control._size(control.type, 12, 16, 28)
        color: control.mdState.backgroundColor

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: item_state.ctx.color.outline

        layer.enabled: control.enabled && color.a > 0 && !control.flat
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.mdState.elevation
        }

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: item_state.stateLayerOpacity
            color: item_state.stateLayerColor
        }
    }

    function _size(t, small, normal, large) {
        return t == MD.Enum.FABSmall ? small : (t == MD.Enum.FABLarge ? large : normal);
    }

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

    Item {
        visible: false
        state: "show"
        states: [
            State {
                name: "hide"
                when: {
                    const fk = control.flickable;
                    const fk_end = fk && !UT.epsilon_equal(fk.visibleArea.heightRatio, 1.0) && (1.0 - fk.visibleArea.heightRatio - fk.visibleArea.yPosition) * fk.height < 4.0;
                    return !control.visible || fk_end;
                }
                PropertyChanges {
                    control.scale: 0.5
                    control.opacity: 0.0
                }
            },
            State {
                name: "show"
                when: true
                PropertyChanges {
                    control.scale: 1.0
                    control.opacity: 1.0
                }
            }
        ]

        transitions: [
            Transition {
                to: "*"
                ScaleAnimator {
                    target: control
                    duration: 100
                }
                OpacityAnimator {
                    target: control
                    duration: 100
                }
            }
        ]
    }
}
