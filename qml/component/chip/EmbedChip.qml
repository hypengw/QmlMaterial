pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

T.Button {
    id: control

    property MD.StateEmbedChip mdState: MD.StateEmbedChip {
        item: control
    }

    property alias elevated: control.mdState.elevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    checkable: false
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: m_leading_comp && MD.Util.hasIcon(icon) ? 8 : 16
    rightPadding: m_trailing_comp && trailingIconName ? 8 : 16
    spacing: 8

    icon.width: 18
    icon.height: 18
    action: null
    font.capitalization: Font.MixedCase

    property string trailingIconName
    property Component leading: m_leading_comp
    property Component trailing: m_trailing_comp

    Component {
        id: m_leading_comp
        MD.Icon {
            visible: name
            size: {
                const ic = control.action ? control.action.icon : control.icon;
                return Math.min(ic.width, ic.height);
            }
            name: {
                const ic = control.action ? control.action.icon : control.icon;
                return ic.name;
            }
        }
    }

    Component {
        id: m_trailing_comp
        MD.Icon {
            visible: name
            name: control.trailingIconName
            size: {
                const ic = control.action ? control.action.icon : control.icon;
                return Math.min(ic.width, ic.height);
            }
        }
    }

    contentItem: Lite.Row {
        alignment: Qt.AlignHCenter | Qt.AlignVCenter
        opacity: control.mdState.contentOpacity
        spacing: 4

        Loader {
            sourceComponent: control.leading
            Lite.Layout.visibilitySource: item
        }

        MD.Label {
            text: control.text
            verticalAlignment: Text.AlignVCenter
            typescale: MD.Token.typescale.label_large
            wrapMode: Text.NoWrap
            Lite.Layout.fillWidth: true
        }

        Loader {
            sourceComponent: control.trailing
            Lite.Layout.visibilitySource: item
        }
    }

    background: Rectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.mdState.borderWidth
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
    }

}
