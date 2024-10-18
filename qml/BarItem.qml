import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.Basic.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    property alias mdState: item_state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    spacing: 8

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pointSize: MD.Token.typescale.label_large.size
    property int lineHeight: MD.Token.typescale.label_large.line_height

    contentItem: Item {
        implicitWidth: children[0].implicitWidth
        implicitHeight: children[0].implicitHeight
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 4

            MD.Icon {
                Layout.fillWidth: true
                implicitWidth: 64
                implicitHeight: 32
                name: control.icon.name
                size: control.icon.width
                color: control.mdState.supportTextColor
            }

            MD.Text {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                font.capitalization: Font.Capitalize
                typescale: MD.Token.typescale.label_medium
                text: control.text
                prominent: control.checked
                color: control.mdState.textColor
            }
        }
    }

    background: Item {
        implicitWidth: 64
        implicitHeight: 32

        MD.ElevationRectangle {
            x: (parent.width - width) / 2
            y: 0
            height: 32
            width: 64

            NumberAnimation on width {
                alwaysRunToEnd: true
                from: 48
                to: 64
                duration: 100
                running: control.checked
            }

            radius: height / 2
            color: control.mdState.backgroundColor
            elevation: control.mdState.elevation
        }
        MD.Ripple2 {
            readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
            x: (parent.width - width) / 2
            y: 0
            height: 32
            width: 64
            radius: height / 2
            pressX: p.x
            pressY: p.y
            pressed: control.pressed
            stateOpacity: item_state.stateLayerOpacity
            color: item_state.stateLayerColor
        }
    }

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level0
        textColor: control.checked ? item_state.ctx.color.on_surface : item_state.ctx.color.on_surface_variant
        backgroundColor: control.checked ? item_state.ctx.color.secondary_container : "transparent"
        supportTextColor: control.checked ? item_state.ctx.color.on_secondary_container : item_state.ctx.color.on_surface_variant
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Pressed"
                when: control.down || control.visualFocus
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: control.checked ? item_state.ctx.color.on_secondary_container : item_state.ctx.color.on_surface
                    item_state.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: control.checked ? item_state.ctx.color.on_secondary_container : item_state.ctx.color.on_surface
                    item_state.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            }
        ]
    }
}
