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

    ScrollBar.vertical: MD.ScrollBar {}

    Connections {
        target: root
        function onHeaderItemChanged() {
            if (root.headerItem) {
                let old = 0;
                // keep top position
                const slot = function () {
                    const header = root.headerItem;
                    const v = root;
                    if (Math.abs(v.contentY + (old + v.topMargin)) < Number.EPSILON) {
                        v.contentY = -(v.topMargin + header.implicitHeight);
                    }
                    old = header.implicitHeight;
                };
                root.headerItem.implicitHeightChanged.connect(slot);
                slot();
            }
        }
    }
}
