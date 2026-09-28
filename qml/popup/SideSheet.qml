pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.SideSheetBase {
    id: control
    property bool animationsEnabled: true
    property color color: MD.Token.color.surface
    property int elevation: MD.Token.elevation.level0
    property int radius: detached ? MD.Token.shape.corner.large : 0
    property int __revision: 0
    property bool __dragAccepted: false

    sheetItem.contentItem.clip: true
    sheetItem.background: MD.ElevationRectangle {
        color: control.color
        elevation: control.elevation
        corners: control.detached ? MD.Util.corners(control.radius) : control.effectiveEdge === Qt.LeftEdge ? MD.Util.corners(0, control.radius, 0, control.radius) : MD.Util.corners(control.radius, 0, control.radius, 0)
    }

    function __cancelDrag() {
        __dragAccepted = false;
        cancelDrag();
    }
    function __localVector(vector) {
        const origin = mapFromItem(null, 0, 0);
        const point = mapFromItem(null, vector.x, vector.y);
        return Qt.point(point.x - origin.x, point.y - origin.y);
    }
    onTransitionRequested: (revision, target, animate) => {
        m_animation.stop();
        __revision = revision;
        if (animate && animationsEnabled && position !== target) {
            m_animation.from = position;
            m_animation.to = target;
            m_animation.start();
        } else {
            position = target;
            completeTransition(revision);
        }
    }
    onStateChanged: {
        if (dragging)
            m_animation.stop();
    }
    onAnimationsEnabledChanged: {
        if (!animationsEnabled && m_animation.running) {
            m_animation.stop();
            completeTransition(__revision);
        }
    }
    NumberAnimation {
        id: m_animation
        target: control
        property: "position"
        duration: MD.Token.duration.medium2
        easing: MD.Token.easing.standard
        onFinished: control.completeTransition(control.__revision)
    }
    DragHandler {
        parent: control.sheetItem
        target: null
        enabled: control.enabled && control.visible && control.sheetItem.visible && control.draggable && control.Window.window !== null && control.Window.window.visible
        xAxis.enabled: true
        yAxis.enabled: false
        acceptedButtons: Qt.LeftButton
        minimumPointCount: 1
        maximumPointCount: 1
        onEnabledChanged: {
            if (!enabled)
                control.__cancelDrag();
        }
        onActiveChanged: {
            if (active)
                control.__dragAccepted = control.beginDrag();
        }
        onTranslationChanged: delta => {
            if (active && control.__dragAccepted)
                control.dragBy(control.__localVector(delta));
        }
        onCanceled: control.__cancelDrag()
        onGrabChanged: (transition, point) => {
            if (transition === PointerDevice.UngrabExclusive && control.__dragAccepted) {
                control.__dragAccepted = false;
                control.releaseDrag(control.__localVector(point.velocity));
            }
        }
    }
}
