import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
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

    contentItem: ColumnLayout {
        spacing: 4
        MD.Control {
            Layout.alignment: Qt.AlignHCenter
            hoverEnabled: false
            focusPolicy: Qt.NoFocus
            leftInset: 12
            rightInset: 12

            contentItem: MD.Icon {
                name: control.icon.name
                size: control.icon.width
                color: control.mdState.supportTextColor
                fill: control.checked
            }

            background: Item {
                implicitWidth: 56
                implicitHeight: 32
                MD.ElevationRectangle {
                    anchors.centerIn: parent
                    height: parent.height
                    width: 56

                    NumberAnimation on width {
                        alwaysRunToEnd: true
                        from: 48
                        to: 56
                        duration: 200
                        running: control.checked
                    }

                    radius: height / 2
                    color: control.mdState.backgroundColor
                    elevation: control.mdState.elevation
                }
                MD.Ripple2 {
                    readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
                    anchors.fill: parent
                    radius: height / 2
                    pressX: p.x
                    pressY: p.y
                    pressed: control.pressed
                    stateOpacity: item_state.stateLayerOpacity
                    color: item_state.stateLayerColor
                }
            }
        }

        MD.Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            font.capitalization: Font.Capitalize
            typescale: MD.Token.typescale.label_medium
            text: control.text
            prominent: control.checked
        }
    }

    background: Item {}

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
