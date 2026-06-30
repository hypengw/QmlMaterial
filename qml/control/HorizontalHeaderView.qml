pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.HorizontalHeaderView {
    id: control

    property int radius: (syncView as MD.TableView)?.effectiveRadius ?? MD.Token.shape.corner.extra_large
    property color outlineColor: (syncView as MD.TableView)?.outlineColor ?? MD.Token.color.outline_variant
    property int outlineWidth: (syncView as MD.TableView)?.outlineWidth ?? 1

    implicitWidth: syncView ? syncView.width : 0
    // The contentHeight of TableView will be zero at start-up, until the delegate
    // items have been loaded. This means that even if the implicit height of
    // HorizontalHeaderView should be the same as the content height in the end, we
    // need to ensure that it has at least a height of 1 at start-up, otherwise
    // TableView won't bother loading any delegates at all.
    implicitHeight: Math.max(1, contentHeight)

    delegate: MD.HorizontalHeaderViewDelegate { }

    MD.Rectangle {
        parent: control
        anchors.fill: parent
        z: 1000
        corners: MD.Util.corners(control.radius, control.radius, 0, 0)
        color: "transparent"
        border.width: control.outlineWidth
        border.color: control.outlineColor
        enabled: false
    }
}
