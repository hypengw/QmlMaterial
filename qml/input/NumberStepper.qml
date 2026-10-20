import QtQuick
import Qcm.Material as MD

MD.NumberInputBase {
    id: control
    implicitWidth: 18
    implicitHeight: 28
    increaseButton: buttons.up
    decreaseButton: buttons.down
    Accessible.role: Accessible.SpinBox
    Accessible.name: formattedValue
    Accessible.onIncreaseAction: increase()
    Accessible.onDecreaseAction: decrease()
    contentItem: MD.NumberStepperButtons {
        id: buttons
    }
}
