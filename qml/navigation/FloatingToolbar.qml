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
    property MD.FloatingToolbarExit exitBehavior: null
    property int exitEdge: Qt.BottomEdge
    property bool dragToHide: true
    property real exitFlickDeceleration: 1500
    property real exitMaximumFlickVelocity: 2500
    readonly property real exitDistance: Math.max(0, exitEdge === Qt.LeftEdge ? x + width : exitEdge === Qt.RightEdge ? (parent ? parent.width : width) - x : exitEdge === Qt.TopEdge ? y + height : (parent ? parent.height : height) - y)
    readonly property bool exitTransitioning: exitAnimation.running || flingAnimation.running
    readonly property real presentedExitOffset: __exitOffset
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
    Component.onCompleted: {
        __syncExit(false);
        __complete = true;
    }
    property real __exitOffset: 0
    function __syncExit(animate = true) {
        if (!exitAnimation)
            return;
        exitAnimation.stop();
        const target = exitBehavior && exitBehavior.enabled ? exitBehavior.offset : 0;
        if (__exitOffset === target)
            return;
        if (animate && __complete && animationsEnabled && exitBehavior && exitBehavior.enabled && !exitBehavior.active) {
            exitAnimation.from = __exitOffset;
            exitAnimation.to = target;
            exitAnimation.start();
        } else {
            __exitOffset = target;
        }
    }
    function __cancelExitDrag() {
        if (!exitDrag || !flingAnimation)
            return;
        flingAnimation.stop();
        const owner = exitDrag.owner;
        exitDrag.owner = null;
        if (owner)
            owner.reset();
    }
    function __exitAxis(vector) {
        return exitEdge === Qt.LeftEdge ? -vector.x : exitEdge === Qt.RightEdge ? vector.x : exitEdge === Qt.TopEdge ? -vector.y : vector.y;
    }
    function __releaseExit(velocity) {
        if (!exitBehavior)
            return;
        const limit = isFinite(exitMaximumFlickVelocity) ? Math.max(0, exitMaximumFlickVelocity) : 2500;
        const speed = Math.max(-limit, Math.min(limit, velocity));
        if (!animationsEnabled || !isFinite(speed) || Math.abs(speed) <= 1 || exitBehavior.offset <= 0 || exitBehavior.offset >= exitBehavior.distance) {
            exitDrag.owner = null;
            exitBehavior.settle();
            return;
        }
        const deceleration = isFinite(exitFlickDeceleration) ? Math.max(1, exitFlickDeceleration) : 1500;
        flingAnimation.from = exitBehavior.offset;
        flingAnimation.to = exitBehavior.offset + speed * Math.abs(speed) / (2 * deceleration);
        flingAnimation.duration = Math.max(1, Math.round(1000 * Math.abs(speed) / deceleration));
        flingAnimation.start();
    }
    onExitBehaviorChanged: {
        __cancelExitDrag();
        __syncExit(false);
    }
    onExitEdgeChanged: {
        __cancelExitDrag();
        __syncExit(false);
    }
    onAnimationsEnabledChanged: {
        if (!animationsEnabled && flingAnimation) {
            flingAnimation.stop();
            if (exitBehavior)
                exitBehavior.settle();
        }
        __syncExit(false);
    }
    property real __flingOffset: 0
    on__FlingOffsetChanged: {
        if (flingAnimation.running && exitBehavior) {
            exitBehavior.offset = __flingOffset;
            if (__flingOffset <= 0 || __flingOffset >= exitBehavior.distance) {
                flingAnimation.stop();
                exitDrag.owner = null;
                exitBehavior.settle();
            }
        }
    }
    NumberAnimation {
        id: flingAnimation
        target: control
        property: "__flingOffset"
        easing.type: Easing.OutQuad
        onFinished: {
            exitDrag.owner = null;
            if (control.exitBehavior)
                control.exitBehavior.settle();
        }
    }
    DragHandler {
        id: exitDrag
        property MD.FloatingToolbarExit owner: null
        target: null
        enabled: control.dragToHide && control.enabled && control.visible && control.exitBehavior !== null && control.exitBehavior.enabled
        xAxis.enabled: control.exitEdge === Qt.LeftEdge || control.exitEdge === Qt.RightEdge
        yAxis.enabled: !xAxis.enabled
        acceptedButtons: Qt.LeftButton
        minimumPointCount: 1
        maximumPointCount: 1
        onEnabledChanged: {
            if (!enabled)
                control.__cancelExitDrag();
        }
        onActiveChanged: {
            if (active) {
                owner = control.exitBehavior;
                owner.begin();
            }
        }
        onTranslationChanged: delta => {
            if (active && owner)
                owner.offset += control.__exitAxis(delta);
        }
        onCanceled: control.__cancelExitDrag()
        onGrabChanged: (transition, point) => {
            if (transition === PointerDevice.UngrabExclusive && owner) {
                control.__releaseExit(control.__exitAxis(point.velocity));
            }
        }
    }
    NumberAnimation {
        id: exitAnimation
        target: control
        property: "__exitOffset"
        duration: control.duration
        easing: MD.Token.easing.emphasized
    }
    Connections {
        target: control.exitBehavior
        function onDistanceChanged() {
            if (flingAnimation.running) {
                flingAnimation.stop();
                exitDrag.owner = null;
                control.exitBehavior.settle();
            }
            control.__syncExit(false);
        }
        function onOffsetChanged() {
            control.__syncExit();
        }
        function onEnabledChanged() {
            if (!control.exitBehavior.enabled)
                flingAnimation.stop();
            control.__syncExit(false);
        }
        function onInputStarted() {
            flingAnimation.stop();
            exitAnimation.stop();
            if (!exitDrag.active)
                exitDrag.owner = null;
            control.exitBehavior.offset = control.__exitOffset;
        }
    }
    transform: Translate {
        x: control.exitEdge === Qt.LeftEdge ? -control.__exitOffset : control.exitEdge === Qt.RightEdge ? control.__exitOffset : 0
        y: control.exitEdge === Qt.TopEdge ? -control.__exitOffset : control.exitEdge === Qt.BottomEdge ? control.__exitOffset : 0
    }

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
