import QtQuick

import Qcm.Material as MD

MD.DialogButtonBoxBase {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    spacing: 8
    horizontalPadding: 24
    alignment: Qt.AlignRight

    delegate: MD.Button {
        mdState.type: MD.Enum.BtText
    }

    background: Item {}
}
