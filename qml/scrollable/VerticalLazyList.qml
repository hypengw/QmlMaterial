pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.LazyList {
    id: root
    property bool busy: false
    property bool expand: false
    property Component header: null
    property Component footer: MD.ListBusyFooter { running: root.busy }
    readonly property Item headerItem: decoration.headerItem
    readonly property Item footerItem: decoration.footerItem
    property real extraLeftMargin: 0
    property real extraRightMargin: 0
    property real extraTopMargin: 0
    property real extraBottomMargin: 0

    implicitHeight: expand ? contentHeight + topMargin + bottomMargin : 0
    onItemsNeeded: demand => {
        if (source instanceof MD.ItemModelSource) source.requestMore(demand);
    }
    clip: true
    synchronousDrag: true
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    MD.NestedScroll.enabled: true
    MD.NestedScroll.axes: Qt.Vertical
    MD.NestedScroll.pressDelay: MD.Token.flick.pressDelay
    MD.ScrollBarBase.vertical: MD.ScrollBar {}

    leftMargin: extraLeftMargin
    rightMargin: extraRightMargin
    topMargin: extraTopMargin + (headerItem?.height ?? 0)
    bottomMargin: extraBottomMargin + (footerItem?.height ?? 0)
    MD.LazyViewContent {
        id: decoration
        view: root
        header: root.header
        footer: root.footer
    }
}
