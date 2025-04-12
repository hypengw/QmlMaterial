import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateInputChip mdState: MD.StateInputChip {
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
    leftPadding: 8
    rightPadding: MD.Util.hasIcon(icon) ? 8 : 16
    spacing: 8

    icon.width: 18
    icon.height: 18
    icon.name: MD.Token.icon.close
    action: null
    font.capitalization: Font.Capitalize

    property Item leadingItem: null

    contentItem: Row {
        opacity: control.mdState.contentOpacity
        Item {
            id: m_leading
            anchors.verticalCenter: parent.verticalCenter
            implicitWidth: children[0].implicitWidth
            implicitHeight: children[0].implicitHeight

            states: State {
                when: !control.leadingItem
                name: "hidden"
                PropertyChanges {
                    control.leftPadding: 16
                    m_leading.implicitWidth: 0
                    m_leading.opacity: 0
                    m_leading.visible: false
                }
            }

            RowLayout {
                spacing: 0
                Row {
                    Layout.alignment: Qt.AlignVCenter
                    MD.ItemHolder {
                        item: control.leadingItem
                    }
                }
                MD.Space {
                    spacing: control.spacing
                }
            }
        }
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            MD.Label {
                Layout.alignment: Qt.AlignVCenter
                text: control.text
                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.label_large
            }
            Item {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: control.action ? control.action.icon.width : control.icon.width
                implicitHeight: control.action ? control.action.icon.height : control.icon.height

                MD.StandardIconButton {
                    anchors.centerIn: parent

                    implicitBackgroundSize: 0
                    visible: action ? action.icon.name : icon.name
                    icon: action ? action.icon : control.icon
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

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

    MD.StateHolder {
        state: control.mdState
    }
}
