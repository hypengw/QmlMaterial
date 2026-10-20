import QtQuick
import Qcm.Material as MD

MD.NumberInputBase {
    id: control
    property int valueWidth: 28
    from: 0
    to: 100
    implicitWidth: valueText.implicitWidth + valueWidth + 22
    implicitHeight: 24
    editor: input
    increaseButton: buttons.up
    decreaseButton: buttons.down
    Accessible.role: Accessible.SpinBox
    Accessible.name: formattedValue
    Accessible.onIncreaseAction: increase()
    Accessible.onDecreaseAction: decrease()

    background: Rectangle {
        color: MD.MProp.color.surface_container_highest
        radius: 4
    }
    contentItem: Item {
        MD.TextFieldBase {
            id: input
            anchors.left: parent.left
            anchors.right: buttons.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 6
            anchors.rightMargin: 2
            height: parent.height
            text: control.formattedValue
            horizontalAlignment: TextInput.AlignRight
            verticalAlignment: TextInput.AlignVCenter
            color: MD.MProp.color.on_surface
            font.pixelSize: MD.Token.typescale.label_small.size
            selectByMouse: true
            clip: true
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            validator: DoubleValidator {
                bottom: Math.min(control.from, control.to)
                top: Math.max(control.from, control.to)
                decimals: control.decimals
                locale: "C"
            }
            MD.Text {
                id: valueText
                visible: false
                text: "999"
                font: input.font
            }
        }
        MD.NumberStepperButtons {
            id: buttons
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 2
            width: 14
            height: parent.height - 4
        }
    }
}
