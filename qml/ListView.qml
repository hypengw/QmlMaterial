import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    clip: true

    property bool busy: false

    // contentHeight: contentItem.childrenRect.height
    contentWidth: width - rightMargin - leftMargin

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0

    footer: MD.ListBusyFooter {
        running: root.busy
        width: ListView.view.width
    }

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
