import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.ButtonBase {
    id: control

    readonly property MD.SegmentedButtonGroup segmentedGroup: MD.ContainerBase.container as MD.SegmentedButtonGroup
    property int position: !segmentedGroup || segmentedGroup.count === 1 ? MD.Enum.PosSingle : MD.ContainerBase.index === 0 ? MD.Enum.PosFirst : MD.ContainerBase.index === segmentedGroup.count - 1 ? MD.Enum.PosLast : MD.Enum.PosMiddle
    property int size: segmentedGroup?.size ?? MD.Enum.S
    property alias mdState: m_state

    MD.StateSegmentedButton {
        id: m_state
        item: control
        position: control.position
        size: control.size
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    verticalPadding: 0
    leftPadding: mdState.leadingSpace
    rightPadding: mdState.trailingSpace
    spacing: mdState.spacing

    checkable: true

    icon.width: mdState.iconSize
    icon.height: mdState.iconSize
    icon.color: control.mdState.textColor

    property MD.typescale typescale: mdState.typescale
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    z: checked ? 1 : 0

    contentItem: Lite.Box {
        alignment: Qt.AlignCenter
        opacity: control.mdState.contentOpacity

        Lite.Row {
            width: Math.min(implicitWidth, parent.width)
            height: Math.min(implicitHeight, parent.height)
            alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: control.spacing

            Item {
                visible: control.checked || !control.icon.empty
                implicitWidth: control.icon.width
                implicitHeight: control.icon.height
                MD.Icon {
                    anchors.centerIn: parent
                    visible: control.checked
                    name: "check"
                    size: Math.min(parent.width, parent.height)
                    color: control.mdState.textColor
                }
                MD.IconView {
                    anchors.fill: parent
                    visible: !control.checked
                    icon: control.icon
                }
            }

            MD.Label {
                text: control.text
                color: control.mdState.textColor
                useTypescale: false
                font: control.font
                lineHeight: control.typescale.line_height
                wrapMode: Text.NoWrap
                Lite.Layout.fillWidth: true
            }
        }
    }

    background: MD.Rectangle {
        implicitWidth: control.mdState.containerHeight
        implicitHeight: control.mdState.containerHeight

        corners: control.mdState.corners
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.mdState.outlineWidth
        border.color: control.mdState.outlineColor

        MD.Ripple {
            anchors.fill: parent
            corners: parent.corners
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }
}
