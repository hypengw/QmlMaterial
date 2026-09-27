import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    property bool pressed: false
    property bool hovered: false
    property bool visualFocus: false
    property bool interactive: true

    property int orientation: Qt.Vertical
    property int controlHeight: 0
    property int controlWidth: 0

    property alias mdState: m_state

    color: mdState.backgroundColor
    implicitWidth: orientation === Qt.Horizontal ? 24 : controlWidth
    implicitHeight: orientation === Qt.Horizontal ? controlHeight : 24

    containmentMask: Item {
        x: (root.width) / 2 - root.mdState.handlePressedWidth / 2
        y: (root.height) / 2 - root.mdState.handlePressedHeight / 2
        width: root.mdState.handlePressedWidth
        height: root.mdState.handlePressedHeight
    }

    onHoveredChanged: {
        if (hovered && interactive) {
            MD.Util.setCursor(root, Qt.OpenHandCursor);
        }
    }
    onPressedChanged: {
        MD.Util.setCursor(root, !interactive ? Qt.ArrowCursor : pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor);
    }
    onInteractiveChanged: MD.Util.setCursor(root, interactive && hovered ? Qt.OpenHandCursor : Qt.ArrowCursor)

    Rectangle {
        id: m_bar
        property int handleWidth: root.mdState.handleWidth
        property int handleHeight: root.mdState.handleHeight
        width: root.orientation === Qt.Horizontal ? handleWidth : handleHeight
        height: root.orientation === Qt.Horizontal ? handleHeight : handleWidth
        color: root.mdState.textColor
        radius: root.mdState.radius
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.mdState.stateLayerColor
            opacity: root.mdState.stateLayerOpacity
        }
        Behavior on handleHeight {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.linear
            }
        }
        Behavior on handleWidth {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.linear
            }
        }
    }

    MD.StateDragHandle {
        id: m_state
        item: root
        pressed: root.pressed
        hovered: root.hovered
        visualFocus: root.visualFocus
    }
}
