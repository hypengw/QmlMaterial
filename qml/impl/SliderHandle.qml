import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 20
    implicitHeight: 20

    property real value: 0
    property bool handleHasFocus: false
    property bool handlePressed: false
    property bool handleHovered: false
    readonly property int initialSize: 4
    readonly property var control: parent

    Rectangle {
        id: handleRect
        anchors.centerIn: parent
        width: 20
        height: 20
        radius: width / 2
        color: root.control ? root.control.MD.MatProp.backgroundColor : "transparent"

        layer.enabled: true
        layer.effect: MD.RoundedElevationEffect {
            elevation: MD.Token.elevation.level1
        }
    }

    MD.Ripple2 {
        anchors.centerIn: parent
        width: 36
        height: 36
        radius: width / 2
        pressed: root.handlePressed || root.handleHasFocus
        color: root.control.MD.MatProp.backgroundColor
        stateOpacity: root.handleHovered ? MD.Token.state.hover.state_layer_opacity : "transparent"
    }
}
