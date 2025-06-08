pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.MenuItem {
    id: control

    property alias mdState: item_state

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 16
    verticalPadding: 0
    spacing: 16

    icon.width: 24
    icon.height: 24
    icon.color: mdState.textColor

    /*
    indicator: CheckIndicator {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checkable
        control: control
        checkState: control.checked ? Qt.Checked : Qt.Unchecked
    }
    */

    arrow: MD.Icon {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2

        visible: control.subMenu
        size: 24
        name: MD.Token.icon.arrow_right
    }

    font.capitalization: Font.Capitalize
    contentItem: MD.IconLabel {

        //readonly property real arrowPadding: control.subMenu && control.arrow ? control.arrow.width + control.spacing : 0
        //readonly property real indicatorPadding: control.checkable && control.indicator ? control.indicator.width + control.spacing : 0
        //leftPadding: !control.mirrored ? indicatorPadding : arrowPadding
        //rightPadding: control.mirrored ? indicatorPadding : arrowPadding

        horizontalAlignment: Qt.AlignLeft
        spacing: control.spacing

        text: control.text
        font: control.font
        typescale: MD.Token.typescale.label_large
        color: control.mdState.textColor

        icon_name: control.icon.name
        icon_size: control.icon.width
        icon_color: control.leadingIconColor
        icon_fill: control.checked

        icon_component: m_loading_comp
        icon_component_active: {
            const a = control.action;
            return a instanceof MD.Action && (a as MD.Action).busy;
        }

        Component {
            id: m_loading_comp
            MD.CircularIndicator {
                anchors.centerIn: parent
                running: true
                strokeWidth: 2
                implicitWidth: {
                    const w = Math.min(control.icon.width, control.icon.height);
                    return w - 6;
                }
                implicitHeight: implicitWidth
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 48
        color: "transparent"

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

    property color leadingIconColor: item_state.leadingIconColor
    property color trailingIconColor: item_state.trailingIconColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level2
        textColor: item_state.ctx.color.on_surface
        backgroundColor: item_state.ctx.color.surface_container
        supportTextColor: item_state.ctx.color.on_surface_variant
        stateLayerColor: "transparent"
        property color leadingIconColor: item_state.ctx.color.on_surface
        property color trailingIconColor: item_state.ctx.color.on_surface

        states: [
            State {
                name: "Disabled"
                when: !control.enabled
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level0
                    control.contentItem.opacity: 0.38
                }
            },
            State {
                name: "Pressed"
                when: control.down || control.visualFocus
                PropertyChanges {
                    item_state.leadingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.trailingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.stateLayerOpacity: MD.Token.state.pressed.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.leadingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.trailingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.stateLayerOpacity: MD.Token.state.hover.state_layer_opacity
                    item_state.stateLayerColor: item_state.ctx.color.on_surface
                }
            }
        ]
    }
}
