pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Button {
    id: root
    required property string title
    required property string status
    required property string symbol
    property bool active: false
    property bool available: true
    property bool dualTarget: true
    property bool toggleable: true
    property string sideSymbol: ''
    readonly property color contentColor: !available ? Qt.rgba(MD.MProp.color.on_surface_variant.r, MD.MProp.color.on_surface_variant.g, MD.MProp.color.on_surface_variant.b, 0.38) : active && !dualTarget ? MD.MProp.color.on_primary : MD.MProp.color.on_surface
    enabled: available
    signal toggleRequested
    signal detailsRequested

    implicitWidth: 256
    implicitHeight: 72
    padding: 8
    leftPadding: 8
    rightPadding: dualTarget ? 8 : 12
    topPadding: 8
    bottomPadding: 8
    checkable: !dualTarget && toggleable
    checked: active
    mdState.type: MD.Enum.BtText
    Accessible.name: title + (status.length ? ', ' + status : '')
    onClicked: {
        if (dualTarget)
            detailsRequested();
        else
            toggleRequested();
    }
    background: Rectangle {
        radius: root.available && root.active ? 24 : 50
        color: !root.available ? Qt.rgba(root.MD.MProp.color.surface.r, root.MD.MProp.color.surface.g, root.MD.MProp.color.surface.b, 0.18) : root.active && !root.dualTarget ? root.MD.MProp.color.primary : Qt.rgba(root.MD.MProp.color.surface_container_high.r, root.MD.MProp.color.surface_container_high.g, root.MD.MProp.color.surface_container_high.b, 0.82)
        Behavior on radius {
            NumberAnimation {
                duration: 300
            }
        }
        MD.Ripple {
            objectName: 'tileRipple'
            anchors.fill: parent
            corners: MD.Util.corners(parent.radius)
            pressX: root.pressX
            pressY: root.pressY
            pressed: root.pressed
            stateOpacity: root.mdState.stateLayerOpacity
            color: root.contentColor
        }
        MD.FocusIndicator {
            corners: MD.Util.corners(parent.radius)
            active: root.visualFocus
        }
    }
    contentItem: Item {
        MD.IconButton {
            objectName: 'toggleButton'
            visible: root.dualTarget
            width: 56
            height: 56
            padding: 0
            topInset: 0
            bottomInset: 0
            leftInset: 0
            rightInset: 0
            anchors.verticalCenter: parent.verticalCenter
            checkable: true
            checked: root.active
            icon.name: root.symbol
            mdState.type: MD.Enum.IBtFilled
            icon.width: 28
            icon.height: 28
            mdState.elevation: 0
            mdState.corners: MD.Util.corners(root.available && root.active ? 16 : 50)
            mdState.backgroundColor: !root.available ? root.MD.MProp.color.surface : root.active ? root.MD.MProp.color.primary : root.MD.MProp.color.surface_container_highest
            mdState.textColor: root.dualTarget && root.active && root.available ? root.MD.MProp.color.on_primary : root.contentColor
            mdState.backgroundOpacity: root.dualTarget ? 1 : 0
            mdState.contentOpacity: 1
            Accessible.name: root.title
            MD.ToolTip.text: (root.active ? 'Turn off ' : 'Turn on ') + root.title
            MD.ToolTip.visible: hovered
            onClicked: root.toggleRequested()
        }
        MD.Icon {
            visible: !root.dualTarget
            width: 56
            height: 56
            size: 28
            name: root.symbol
            color: root.contentColor
        }
        Item {
            id: details
            objectName: 'tileLabels'
            x: 62
            width: parent.width - x
            height: parent.height
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, parent.width - (root.sideSymbol.length ? 38 : 0))
                spacing: 0
                MD.Text {
                    objectName: 'tileLabel'
                    width: parent.width
                    text: root.title
                    typescale: MD.Token.typescale.title_small
                    font.weight: Font.Bold
                    color: root.contentColor
                    wrapMode: Text.NoWrap
                }
                MD.Text {
                    objectName: 'tileSecondaryLabel'
                    visible: text.length > 0
                    width: parent.width
                    text: root.status
                    color: root.contentColor
                    typescale: MD.Token.typescale.label_medium
                    wrapMode: Text.NoWrap
                }
            }
            MD.Icon {
                objectName: 'tileChevron'
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: root.sideSymbol.length > 0
                name: root.sideSymbol
                color: root.contentColor
                size: 20
                width: 32
                height: 20
            }
        }
    }
    readonly property alias detailsAnchor: root
}
