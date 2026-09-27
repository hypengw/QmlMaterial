import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.TabButtonBase {
    id: control

    property int type: MD.TabBar.tabBar?.type ?? MD.Enum.PrimaryTab
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: !icon.empty

    property MD.StateTabButton mdState: MD.StateTabButton {
        item: control
    }

    Binding {
        control.mdState.type: control.type
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    spacing: 8

    leftPadding: 12
    rightPadding: 12

    icon.width: 24
    icon.height: 24
    icon.color: control.mdState.textColor

    property MD.typescale typescale: MD.Token.typescale.label_medium
    font.capitalization: Font.MixedCase
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: Lite.Box {
        alignment: Qt.AlignCenter
        opacity: control.mdState.contentOpacity

        Lite.Row {
            width: Math.min(implicitWidth, parent.width)
            height: Math.min(implicitHeight, parent.height)
            alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: control.spacing

            MD.IconView {
                visible: control.iconStyle != MD.Enum.TextOnly && control.hasIcon
                icon: control.icon
            }

            MD.Label {
                visible: control.iconStyle != MD.Enum.IconOnly
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

    background: MD.Ripple {
        implicitHeight: 48

        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: control.mdState.stateLayerOpacity
        color: control.mdState.stateLayerColor
        // opacity: control.mdState.backgroundOpacity
    }
}
