import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.SplitView {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    handle: MD.DragHandle {
        pressed: T.SplitHandle.pressed
        hovered: T.SplitHandle.hovered
        visualFocus: control.visualFocus
        orientation: control.orientation
        controlHeight: control.height
        controlWidth: control.width
    }
}
