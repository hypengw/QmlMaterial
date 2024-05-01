import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

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
                color: control.MD.MatProp.supportTextColor
            }

            MD.Text {
                Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                font.capitalization: Font.Capitalize
                typescale: MD.Token.typescale.label_medium
                text: control.text
                prominent: control.checked
            }
        }
    }

    background: Item {
        implicitWidth: 64
        implicitHeight: 32

        Rectangle {
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
            color: control.MD.MatProp.backgroundColor

            layer.enabled: control.enabled && color.a > 0 && !control.flat
            layer.effect: MD.RoundedElevationEffect {
                elevation: MD.Token.elevation.level0//control.MD.MatProp.elevation
            }

            MD.Ripple {
                clip: true
                clipRadius: parent.radius
                width: parent.width
                height: parent.height
                pressed: control.pressed
                anchor: control
                active: enabled && (control.down || control.visualFocus || control.hovered)
                color: control.MD.MatProp.stateLayerColor
            }
        }
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor // icon color
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level1
        textColor: control.checked ? item_state.ctx.color.on_surface : item_state.ctx.color.on_surface_variant
        backgroundColor: control.checked ? item_state.ctx.color.secondary_container : "transparent"
        supportTextColor: control.checked ? item_state.ctx.color.on_secondary_container : item_state.ctx.color.on_surface_variant
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Pressed"
                when: control.down || control.focus
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level1
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: control.checked ? item_state.ctx.color.on_secondary_container : item_state.ctx.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = item_state.ctx.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level2
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.supportTextColor: control.checked ? item_state.ctx.color.on_secondary_container : item_state.ctx.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = item_state.ctx.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
