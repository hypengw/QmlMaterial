import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.TabButtonBase {
    id: control

    property int type: MD.TabBar.tabBar?.type ?? MD.Enum.PrimaryTab
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    property bool inlineLabel: true
    readonly property bool _stacked: !inlineLabel && iconStyle === MD.Enum.IconAndText && hasIcon && text.length > 0
    readonly property bool hasIcon: !icon.empty

    property MD.StateTabButton mdState: MD.StateTabButton {
        item: control
    }

    readonly property QtObject _motion: QtObject {
        property bool enabled: false
        property color textColor: control.mdState.textColor
        Behavior on textColor {
            enabled: control._motion.enabled
            ColorAnimation {
                duration: control.checked ? MD.Token.duration.short4 : MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
        }
    }
    Component.onCompleted: _motion.enabled = true

    Binding {
        control.mdState.type: control.type
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    spacing: _stacked ? 4 : 8

    leftPadding: 16
    rightPadding: 16

    icon.width: 24
    icon.height: 24
    icon.color: control._motion.textColor

    property MD.typescale typescale: MD.Token.typescale.title_small
    font.capitalization: Font.MixedCase
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    contentItem: Lite.Box {
        alignment: Qt.AlignCenter
        opacity: control.mdState.contentOpacity

        Lite.Row {
            id: m_row
            visible: !control._stacked
            width: Math.min(implicitWidth, parent.width)
            height: Math.min(implicitHeight, parent.height)
            alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: control.spacing

            MD.IconView {
                parent: control._stacked ? m_column : m_row
                visible: control.iconStyle != MD.Enum.TextOnly && control.hasIcon
                icon: control.icon
            }

            MD.Label {
                parent: control._stacked ? m_column : m_row
                visible: control.iconStyle != MD.Enum.IconOnly
                text: control.text
                color: control._motion.textColor
                useTypescale: false
                font: control.font
                lineHeight: control.typescale.line_height
                wrapMode: Text.NoWrap
                Lite.Layout.fillWidth: true
            }
        }
        Lite.Column {
            id: m_column
            visible: control._stacked
            width: Math.min(implicitWidth, parent.width)
            height: Math.min(implicitHeight, parent.height)
            alignment: Qt.AlignHCenter | Qt.AlignVCenter
            spacing: control.spacing
        }
    }

    background: MD.Ripple {
        implicitHeight: control._stacked ? 72 : 48

        pressX: control.pressX
        pressY: control.pressY
        pressed: control.pressed
        stateOpacity: control.mdState.stateLayerOpacity
        color: control.mdState.stateLayerColor
        // opacity: control.mdState.backgroundOpacity
    }
}
