pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.SplitViewBase {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    property real handleOpacity: 1

    handle: MD.DragHandle {
        id: m_handle
        containmentMask: null
        opacity: control.handleOpacity
        pressed: MD.SplitHandle.pressed
        hovered: MD.SplitHandle.hovered
        visualFocus: control.visualFocus
        orientation: control.orientation
        controlHeight: control.height
        controlWidth: control.width
    }
}
