pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.Control {
    id: control
    property MD.TimeState time: MD.TimeState {
        locale: control.locale
    }
    property bool showModeToggle: true
    property string supportingText: qsTr("Select time")
    readonly property int __displayMode: time.displayMode
    property bool __complete: false
    Component.onCompleted: __complete = true
    on__DisplayModeChanged: {
        if (!__complete || !visible)
            return;
        if (__displayMode === MD.TimeState.Input)
            input.focusSelection();
        else
            dial.forceActiveFocus(Qt.OtherFocusReason);
    }
    readonly property MD.time_picker_token tokens: MD.Token.time_picker
    padding: tokens.padding
    implicitWidth: Math.max(input.implicitWidth, tokens.dialSize) + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    contentItem: Item {
        implicitHeight: toggle.y + (toggle.visible ? toggle.height : 0)
        MD.Label {
            id: heading
            width: parent.width
            text: control.supportingText
            typescale: MD.Token.typescale.label_medium
            color: MD.Token.color.on_surface_variant
        }
        MD.TimeInput {
            id: input
            y: heading.height + 20
            width: parent.width
            height: implicitHeight
            time: control.time
            layoutDirection: control.layoutDirection
            editable: control.time.displayMode === MD.TimeState.Input
            onInputRequested: control.time.displayMode = MD.TimeState.Input
        }
        MD.TimeDial {
            id: dial
            opacity: enabled ? 1 : 0.38
            objectName: "timeDial"
            visible: control.time.displayMode === MD.TimeState.Clock
            time: control.time
            width: Math.min(parent.width, control.tokens.dialSize)
            height: width
            x: (parent.width - width) / 2
            y: input.y + input.height + control.tokens.dialGap
            property bool ready: false
            Component.onCompleted: ready = true
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: MD.Token.color.surface_container_highest
            }
            Item {
                x: dial.width / 2
                y: dial.height / 2
                rotation: dial.handAngle
                Behavior on rotation {
                    enabled: dial.ready && dial.visible && !dial.pressed
                    RotationAnimation {
                        direction: RotationAnimation.Shortest
                        duration: MD.Token.duration.short3
                        easing: MD.Token.easing.standard
                    }
                }
                Rectangle {
                    x: -width / 2
                    y: -height
                    width: control.tokens.handWidth
                    height: dial.handLength
                    color: MD.Token.color.primary
                }
                Rectangle {
                    x: -width / 2
                    y: -dial.handLength - height / 2
                    width: control.tokens.selectorSize
                    height: width
                    radius: width / 2
                    color: MD.Token.color.primary
                }
            }
            Rectangle {
                anchors.centerIn: parent
                width: control.tokens.centerSize
                height: width
                radius: width / 2
                color: MD.Token.color.primary
            }
            Repeater {
                model: dial.labels
                delegate: MD.Label {
                    required property var modelData
                    x: modelData.x - width / 2
                    y: modelData.y - height / 2
                    width: control.tokens.selectorSize
                    height: control.tokens.selectorSize
                    text: modelData.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    typescale: MD.Token.typescale.body_large
                    color: modelData.selected ? MD.Token.color.on_primary : MD.Token.color.on_surface
                }
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: width / 2
                color: "transparent"
                border.width: dial.activeFocus ? 2 : 0
                border.color: MD.Token.color.primary
            }
        }
        MD.StandardIconButton {
            id: toggle
            objectName: "timeModeToggle"
            y: dial.visible ? dial.y + dial.height + 12 : input.y + input.height + 12
            visible: control.showModeToggle
            enabled: control.time.acceptableInput
            icon.name: control.time.displayMode === MD.TimeState.Clock ? "keyboard" : "schedule"
            onClicked: control.time.displayMode = control.time.displayMode === MD.TimeState.Clock ? MD.TimeState.Input : MD.TimeState.Clock
            MD.ToolTip.text: control.time.displayMode === MD.TimeState.Clock ? qsTr("Enter time") : qsTr("Select time")
            MD.ToolTip.visible: hovered
        }
    }
}
