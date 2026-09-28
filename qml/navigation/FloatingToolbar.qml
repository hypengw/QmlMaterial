pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup component */
MD.Control {
    id: control

    property int orientation: Qt.Horizontal
    property bool expanded: true
    property Item leadingContent
    property Item mainContent
    property Item trailingContent
    property bool animationsEnabled: true
    property int duration: MD.Token.duration.short4
    property real expansionProgress: motion.progress
    readonly property bool transitioning: animation.running
    property color backgroundColor: MD.MProp.color.surface_container
    property color contentColor: MD.MProp.color.on_surface
    property real elevation: MD.Token.elevation.level0
    property real minimumThickness: MD.Token.floating_toolbar.thickness
    readonly property bool __horizontal: orientation === Qt.Horizontal
    readonly property real __progress: MD.Util.clamp(expansionProgress, 0, 1)
    property bool __complete: false
    Component.onCompleted: __complete = true

    padding: MD.Token.floating_toolbar.padding
    MD.MProp.textColor: contentColor
    MD.MProp.backgroundColor: backgroundColor

    readonly property QtObject __motion: QtObject {
        id: motion
        property real progress: control.expanded ? 1 : 0
        Behavior on progress {
            enabled: control.__complete && control.animationsEnabled
            NumberAnimation {
                id: animation
                duration: control.duration
                easing: MD.Token.easing.emphasized
            }
        }
    }

    readonly property real __naturalAxis: main.axis + (leading.axis + trailing.axis) * __progress
    readonly property real __naturalCross: Math.max(main.cross, leading.cross * __progress, trailing.cross * __progress)
    implicitWidth: (__horizontal ? __naturalAxis : Math.max(minimumThickness - leftPadding - rightPadding, __naturalCross)) + leftPadding + rightPadding
    implicitHeight: (__horizontal ? Math.max(minimumThickness - topPadding - bottomPadding, __naturalCross) : __naturalAxis) + topPadding + bottomPadding

    component Slot: Item {
        id: slot
        property Item target
        property bool extension: true
        property bool fromEnd: false
        readonly property real axis: Math.max(0, control.__horizontal ? proxy.implicitWidth : proxy.implicitHeight)
        readonly property real cross: Math.max(0, control.__horizontal ? proxy.implicitHeight : proxy.implicitWidth)
        clip: true
        visible: width > 0 && height > 0 && (!extension || control.__progress > 0)
        enabled: visible && (!extension || control.expanded)
        opacity: extension ? control.__progress : 1

        MD.ItemProxy {
            id: proxy
            active: true
            target: slot.target
            width: control.__horizontal && slot.extension ? implicitWidth : slot.width
            height: !control.__horizontal && slot.extension ? implicitHeight : slot.height
            x: control.__horizontal && slot.fromEnd ? slot.width - width : 0
            y: !control.__horizontal && slot.fromEnd ? slot.height - height : 0
        }
    }

    contentItem: Item {
        id: body
        readonly property real axis: control.__horizontal ? width : height
        readonly property real cross: control.__horizontal ? height : width
        readonly property real mainExtent: Math.min(main.axis, axis)
        readonly property real extensionBudget: Math.max(0, axis - mainExtent)
        readonly property real leadingExtent: leading.axis * control.__progress <= extensionBudget + 0.000001 ? leading.axis * control.__progress : 0
        readonly property real trailingExtent: trailing.axis * control.__progress <= extensionBudget - leadingExtent + 0.000001 ? trailing.axis * control.__progress : 0
        readonly property real offset: Math.max(0, (axis - mainExtent - leadingExtent - trailingExtent) / 2)

        function horizontalPosition(start, extent) {
            return control.mirrored ? width - start - extent : start;
        }

        Slot {
            id: leading
            target: control.leadingContent
            fromEnd: !control.__horizontal || !control.mirrored
            width: control.__horizontal ? body.leadingExtent : Math.min(cross, body.cross)
            height: control.__horizontal ? Math.min(cross, body.cross) : body.leadingExtent
            x: control.__horizontal ? body.horizontalPosition(body.offset, width) : (body.width - width) / 2
            y: control.__horizontal ? (body.height - height) / 2 : body.offset
        }
        Slot {
            id: main
            target: control.mainContent
            extension: false
            width: control.__horizontal ? body.mainExtent : Math.min(cross, body.cross)
            height: control.__horizontal ? Math.min(cross, body.cross) : body.mainExtent
            x: control.__horizontal ? body.horizontalPosition(body.offset + body.leadingExtent, width) : (body.width - width) / 2
            y: control.__horizontal ? (body.height - height) / 2 : body.offset + body.leadingExtent
        }
        Slot {
            id: trailing
            target: control.trailingContent
            fromEnd: control.__horizontal && control.mirrored
            width: control.__horizontal ? body.trailingExtent : Math.min(cross, body.cross)
            height: control.__horizontal ? Math.min(cross, body.cross) : body.trailingExtent
            x: control.__horizontal ? body.horizontalPosition(body.offset + body.leadingExtent + body.mainExtent, width) : (body.width - width) / 2
            y: control.__horizontal ? (body.height - height) / 2 : body.offset + body.leadingExtent + body.mainExtent
        }
    }

    background: MD.ElevationRectangle {
        radius: Math.min(width, height) / 2
        color: control.backgroundColor
        elevation: control.elevation
        elevationVisible: elevation > 0 && color.a > 0
    }
}
