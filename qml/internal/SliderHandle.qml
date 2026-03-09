import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: horizontal ? handleWidth : handleHeight
    implicitHeight: horizontal ? handleHeight : handleWidth

    property real value: 0
    property bool handleHasFocus: false
    property bool handlePressed: false
    property bool handleHovered: false
    property int handleWidth: 12
    property int handleHeight: 44
    property bool horizontal: false
    property int handleLineWidth: 4

    readonly property var control: parent

    // The value indicator (bubble)
    MD.Control {
        y: root.horizontal ? -height - 4 : (parent.height - height) / 2
        x: root.horizontal ? (parent.width - width) / 2 : -width - 4

        visible: root.handlePressed
        opacity: visible ? 1 : 0
        scale: visible ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short2
            }
        }
        Behavior on scale {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
        }

        contentItem: Item {
            implicitHeight: children[0].implicitHeight
            implicitWidth: children[0].implicitWidth
            MD.Text {
                anchors.centerIn: parent
                text: Math.round(root.value)
                typescale: MD.Token.typescale.label_medium
                color: root.control ? root.control.mdState.ctx.color.inverse_on_surface : "transparent"
            }
        }
        background: MD.ElevationRectangle {
            implicitWidth: 32
            implicitHeight: 32
            radius: 16
            color: root.control ? root.control.mdState.ctx.color.inverse_surface : "transparent"
            elevation: MD.Token.elevation.level2
        }
    }

    // The vertical/horizontal line handle (separator)
    Rectangle {
        id: handleRect
        anchors.centerIn: parent

        width: root.horizontal ? root.handleLineWidth : 44
        height: root.horizontal ? 44 : root.handleLineWidth

        radius: 2
        color: root.control ? root.control.mdState.backgroundColor : "transparent"
    }

    // The "pill" outline handle shown when pressed/focused to emphasize
    Rectangle {
        id: pillHandle
        anchors.centerIn: parent
        width: (root.horizontal ? 4 : 44) + 12
        height: (root.horizontal ? 44 : 4) + 12
        radius: (4 + 12) / 2
        color: "transparent"
        border.color: root.control ? root.control.mdState.backgroundColor : "transparent"
        border.width: 4
        visible: root.handleHasFocus

        opacity: visible ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.short2
            }
        }
    }
}
