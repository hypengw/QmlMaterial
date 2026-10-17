pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.SliderBase {
    id: root
    property string label: 'Brightness'
    property string symbol: MD.Token.icon.brightness_6
    readonly property real thumbCenter: 2 + (availableWidth - 4) * position
    readonly property real activeLength: Math.max(0, thumbCenter - 8)
    readonly property real inactiveStart: Math.min(availableWidth, thumbCenter + 8)
    readonly property bool iconOnInactive: availableWidth - inactiveStart > 40
    implicitWidth: 320
    implicitHeight: 64
    padding: 0
    from: 0
    to: 100
    stepSize: 1
    Accessible.name: label
    handle: Rectangle {
        objectName: 'brightnessThumb'
        width: 4
        height: 52
        x: root.visualPosition * (root.availableWidth - width)
        y: (root.height - height) / 2
        radius: 2
        color: root.MD.MProp.color.primary
    }
    background: Item {
        MD.Rectangle {
            objectName: 'brightnessFill'
            width: root.activeLength
            height: 40
            y: (parent.height - height) / 2
            corners: root.mirrored ? MD.Util.corners(2, 12, 2, 12) : MD.Util.corners(12, 2, 12, 2)
            color: root.MD.MProp.color.primary
            x: root.mirrored ? parent.width - width : 0
        }
        MD.Rectangle {
            objectName: 'brightnessTrack'
            width: Math.max(0, root.availableWidth - root.inactiveStart)
            height: 40
            y: (parent.height - height) / 2
            x: root.mirrored ? 0 : root.inactiveStart
            corners: root.mirrored ? MD.Util.corners(12, 2, 12, 2) : MD.Util.corners(2, 12, 2, 12)
            color: root.MD.MProp.color.secondary_container
        }
        MD.Icon {
            objectName: 'brightnessIcon'
            readonly property real logicalX: (root.iconOnInactive ? root.availableWidth : root.activeLength) - 34
            x: root.mirrored ? parent.width - logicalX - width : logicalX
            anchors.verticalCenter: parent.verticalCenter
            size: 28
            visible: root.iconOnInactive || root.activeLength > 40
            name: root.symbol
            color: root.iconOnInactive ? root.MD.MProp.color.on_secondary_container : root.MD.MProp.color.on_primary
        }
        MD.FocusIndicator {
            anchors.fill: parent
            corners: MD.Util.corners(24)
            active: root.visualFocus
        }
        MD.FocusIndicator {
            anchors.fill: parent
            corners: MD.Util.corners(32)
            active: root.visualFocus
        }
    }
}
