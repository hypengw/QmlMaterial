import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    clip: true

    property bool busy: false
    property bool expand: false
    reuseItems: true
    synchronousDrag: true
    pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    cacheBuffer: 96
    rebound: Transition {}
    populate: Transition {}

    // contentHeight/width, managerd by listview itself
    // only set one side
    Binding on contentWidth {
        when: root.orientation === ListView.Vertical
        value: root.width - root.rightMargin - root.leftMargin
    }
    Binding on contentHeight {
        when: root.orientation === ListView.Horizontal
        value: root.height - root.topMargin - root.bottomMargin
    }

    implicitHeight: expand ? contentHeight + topMargin + bottomMargin : 0

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0

    footer: MD.ListBusyFooter {
        running: root.busy
        width: ListView.view.width
        height: implicitHeight
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
