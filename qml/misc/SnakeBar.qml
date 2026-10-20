import QtQuick

import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.SnakeBarBase {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    property alias text: label.text
    property MD.StateSnakeBar mdState: MD.StateSnakeBar {
        item: control
        actionItem: btn_action
        iconItem: btn_icon
    }
    property MD.StateSnakeBar mdStateAction: mdState
    property MD.StateSnakeBar mdStateIcon: mdState
    actionControl: btn_action
    iconControl: btn_icon
    property bool actionOnNewLine: false
    property bool showClose: false
    property real radius: MD.Token.shape.corner.extra_small
    readonly property bool stackedAction: actionOnNewLine && !!action

    padding: 0
    leftPadding: 16
    rightPadding: showClose ? 8 : (action ? 8 : 16)
    topPadding: stackedAction ? 12 : 0
    bottomPadding: stackedAction ? 12 : 0

    background: MD.ElevationRectangle {
        implicitHeight: control.stackedAction ? 112 : Math.max(48, label.implicitHeight + 28)
        color: control.mdState.backgroundColor
        radius: control.radius
        elevation: control.mdState.elevation
    }
    contentItem: Lite.Column {
        alignment: Qt.AlignVCenter
        spacing: control.stackedAction ? 16 : 0

        Lite.Row {
            id: inline_content
            alignment: Qt.AlignVCenter
            spacing: 8
            Lite.Layout.fillWidth: true

            MD.Text {
                id: label

                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.body_medium
                color: control.mdState.supportTextColor
                Lite.Layout.fillWidth: true
            }

            MD.CommandButtonBase {
                id: btn_action
                parent: control.stackedAction ? stacked_action_content : inline_content
                font.capitalization: Font.MixedCase
                visible: !!control.action
                enabled: control.available && !!control.action && control.action.enabled
                text: control.action ? control.action.text : ""

                implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
                implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

                topInset: 0
                bottomInset: 0
                verticalPadding: 0
                leftPadding: 12
                rightPadding: 12
                spacing: 8

                contentItem: Lite.Box {
                    alignment: Qt.AlignCenter

                    MD.Label {
                        text: btn_action.text
                        color: control.mdState.textColor
                        typescale: MD.Token.typescale.label_large
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.NoWrap
                        Lite.Layout.fillWidth: true
                    }
                }

                background: Rectangle {
                    implicitWidth: 64
                    implicitHeight: 48 - 8 * 2
                    radius: height / 2
                    border.width: 0
                    color: "transparent"

                    MD.Ripple {
                        anchors.fill: parent
                        radius: parent.radius
                        interactionSource: btn_action.interactionSource
                        indication: control.mdState.actionIndication
                        color: control.mdState.stateLayerColor
                    }
                }
            }

            MD.CommandButtonBase {
                id: btn_icon
                visible: control.showClose
                enabled: control.available

                implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
                implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

                topInset: 0
                bottomInset: 0
                rightInset: 0
                leftInset: 0
                padding: 0
                spacing: 8

                contentItem: Item {
                    implicitWidth: 24
                    implicitHeight: 24
                    MD.Icon {
                        anchors.centerIn: parent
                        name: MD.Token.icon.close
                        size: 24
                        color: control.mdState.iconColor
                    }
                }

                background: Rectangle {
                    implicitWidth: 48 - 8 * 2
                    implicitHeight: 48 - 8 * 2
                    radius: height / 2
                    border.width: 0
                    color: "transparent"

                    MD.Ripple {
                        anchors.fill: parent
                        radius: parent.radius
                        interactionSource: btn_icon.interactionSource
                        indication: control.mdState.iconIndication
                        color: control.mdState.iconStateLayerColor
                    }
                }
            }
        }

        Lite.Box {
            id: stacked_action_content
            visible: control.stackedAction
            alignment: Qt.AlignRight | Qt.AlignVCenter
            Lite.Layout.fillWidth: true
        }
    }
}
