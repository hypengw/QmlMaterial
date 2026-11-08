pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.LazyList {
    id: root
    orientation: Qt.Horizontal
    onItemsNeeded: demand => {
        if (source instanceof MD.ItemModelSource)
            source.requestMore(demand);
    }
    clip: true
    implicitWidth: contentWidth + leftMargin + rightMargin
    synchronousDrag: true
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration
    MD.NestedScroll.enabled: true
    MD.NestedScroll.axes: Qt.Horizontal
    MD.NestedScroll.pressDelay: MD.Token.flick.pressDelay
}
