import QtQuick
import QtQuick.Templates as T
import QtQml.Models
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

T.Button {
    id: control

    property MD.StateInputChip mdState: MD.StateInputChip {
        item: control
    }

    property bool edit: false
    property alias elevated: control.mdState.elevated
    property Item leadingItem: null
    property Component editDelegate: null

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    checkable: false
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: leadingItem ? 8 : 16
    rightPadding: icon.name ? 8 : 16
    spacing: 8

    icon.width: 18
    icon.height: 18

    action: null
    contentItem: contentItemRO

    property Item contentItemRO: Lite.Row {
        alignment: Qt.AlignHCenter | Qt.AlignVCenter
        opacity: control.mdState.contentOpacity
        spacing: control.spacing

        Lite.Box {
            id: m_leading
            alignment: Qt.AlignCenter
            visible: control.leadingItem?.visible ?? false
            data: control.leadingItem ? [control.leadingItem] : []
        }

        Lite.Row {
            alignment: Qt.AlignVCenter
            spacing: control.spacing
            Lite.Layout.fillWidth: true

            MD.Label {
                text: control.text
                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.label_large
                wrapMode: Text.NoWrap
                Lite.Layout.fillWidth: true
            }

            Item {
                implicitWidth: control.icon.width
                implicitHeight: control.icon.height
                visible: control.icon.name

                MD.StandardIconButton {
                    anchors.centerIn: parent

                    implicitBackgroundSize: 0
                    icon: control.icon
                    action: control.action
                }
            }
        }
    }

    background: Rectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.checked ? 0 : 1
        border.color: control.mdState.outlineColor

        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }

        MD.FocusIndicator {
            corners: MD.Util.corners(parent.radius)
            active: control.visualFocus
        }
    }

    onFocusChanged: {
        if (focus)
            control.contentItem.focus = true;
    }

    Instantiator {
        model: control.edit ? 1 : null
        delegate: control.editDelegate
        onObjectAdded: (idx, obj) => {
            control.contentItem = obj;
            obj.focus = true;
        }
        onObjectRemoved: (idx, obj) => {
            control.contentItem = control.contentItemRO;
            control.contentItem.visible = true;
        }
    }
}
