import QtQuick
import Qcm.Material as MD

Item {
    id: control
    readonly property alias up: up
    readonly property alias down: down
    implicitWidth: 18
    implicitHeight: 28

    component ArrowButton: MD.RepeatButtonBase {
        id: button
        property string symbol
        implicitWidth: 18
        implicitHeight: 14
        Accessible.role: Accessible.Button
        Accessible.onPressAction: click()
        MD.InteractionObserver {
            id: interactions
            source: button.interactionSource
        }
        background: Rectangle {
            radius: 4
            color: MD.Util.transparent(MD.MProp.color.on_surface, interactions.pressed ? 0.12 : interactions.hovered ? 0.08 : 0)
        }
        contentItem: MD.Icon {
            name: button.symbol
            size: 12
            color: MD.MProp.color.on_surface_variant
        }
    }
    ArrowButton {
        id: up
        width: control.width
        height: control.height / 2
        symbol: "arrow_drop_up"
        Accessible.name: qsTr("Increase")
    }
    ArrowButton {
        id: down
        y: control.height / 2
        width: control.width
        height: control.height / 2
        symbol: "arrow_drop_down"
        Accessible.name: qsTr("Decrease")
    }
}
