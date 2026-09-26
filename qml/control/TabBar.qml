import QtQuick
import Qcm.Material as MD

MD.TabBarBase {
    id: control

    property int type: MD.Enum.PrimaryTab
    property int radius: 0
    property MD.corners corners: MD.Util.corners(radius)
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    spacing: 1

    contentItem: Flickable {
        id: m_view
        contentWidth: control.contentHost.width
        contentHeight: control.contentHost.height
        clip: true
        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.StopAtBounds
        function revealCurrent() {
            const item = control.currentItem;
            if (!item)
                return;
            const target = item.x < contentX ? item.x : item.x + item.width > contentX + width ? item.x + item.width - width : contentX;
            contentX = Math.max(0, Math.min(Math.max(0, contentWidth - width), target));
        }
        onContentWidthChanged: Qt.callLater(revealCurrent)
        Binding {
            target: control.contentHost
            property: "parent"
            value: m_view.contentItem
        }
        Connections {
            target: control
            function onCurrentItemChanged() {
                Qt.callLater(m_view.revealCurrent);
            }
        }
        Item {
            x: control.currentItem?.x ?? 0
            width: control.currentItem?.width ?? 0
            height: m_view.height
            visible: control.currentItem !== null
            z: 2
            Behavior on x {
                NumberAnimation {
                    duration: 250
                }
            }
            Item {
                x: control.type == MD.Enum.PrimaryTab ? (parent.width - width) / 2 : 0
                y: control.position === MD.TabBar.Footer ? 0 : parent.height - height
                height: control.type == MD.Enum.PrimaryTab ? 3 : 2
                width: control.type == MD.Enum.PrimaryTab ? (control.currentItem?.implicitContentWidth ?? 0) : parent.width
                clip: true

                Behavior on width {
                    NumberAnimation {
                        duration: 250
                    }
                }
                Rectangle {
                    height: parent.height * 2
                    width: parent.width
                    radius: control.type == MD.Enum.PrimaryTab ? 3 : 0
                    color: MD.Token.color.primary
                }
            }
        }
    }

    background: MD.Rectangle {
        color: control.MD.MProp.backgroundColor
        corners: control.corners

        //layer.enabled: control.Material.elevation > 0
        //layer.effect: MD.ElevationEffect {
        //    elevation: control.Material.elevation
        //    fullWidth: true
        //}

        MD.AutoDivider {
            anchors.bottom: parent.bottom
        }
    }

    layer.enabled: true
    //layer.sourceRect: Qt.rect(0, 0, control.width, control.height + 8)
    //layer.textureSize: Qt.size(control.width, control.height + 8)
    layer.effect: Item {
        property var source
        MD.RoundClip {
            source: parent.source
            width: parent.width
            height: parent.height
            corners: control.corners
            size: Qt.vector2d(control.width, control.height)
        }
    }
}
