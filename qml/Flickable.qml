import QtQuick

import Qcm.Material as MD

Flickable {
    id: root

    // it's hard to avoid item around Flickable is not transparent
    clip: true

    contentHeight: contentItem.childrenRect.height
    contentWidth: width - rightMargin - leftMargin
    implicitHeight: contentHeight + topMargin + bottomMargin
    implicitWidth: contentWidth + rightMargin + leftMargin
    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0

    signal wheelMoved

    MD.WheelHandler {
        id: wheel
        target: root
        filterMouseEvents: false
        onWheelMoved: root.wheelMoved()
    }
    ScrollBar.vertical: MD.ScrollBar {
    }
}

