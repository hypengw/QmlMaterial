pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    required property MD.Scrollable view
    property Component header: null
    property Component footer: null
    readonly property Item headerItem: headerLoader.item
    readonly property Item footerItem: footerLoader.item
    property real previousTopMargin: 0
    property real previousContentY: 0

    Connections {
        target: root.view
        function onContentYChanged() {
            if (root.view.topMargin === root.previousTopMargin)
                root.previousContentY = root.view.contentY;
        }
        function onTopMarginChanged() {
            // Preserve the top anchor when asynchronous header content changes size.
            if (!root.view.moving && Math.abs(root.previousContentY + root.previousTopMargin) < 0.5)
                root.view.contentY = -root.view.topMargin;
            root.previousTopMargin = root.view.topMargin;
            root.previousContentY = root.view.contentY;
        }
    }
    Component.onCompleted: {
        previousTopMargin = view.topMargin;
        view.contentY = -view.topMargin;
        previousContentY = view.contentY;
    }
    Loader {
        id: headerLoader
        parent: root.view.contentItem
        width: root.view.contentWidth
        height: item?.implicitHeight ?? 0
        y: -height
        sourceComponent: root.header
    }
    Loader {
        id: footerLoader
        parent: root.view.contentItem
        width: root.view.contentWidth
        height: item?.implicitHeight ?? 0
        y: root.view.contentHeight
        sourceComponent: root.footer
    }
}
