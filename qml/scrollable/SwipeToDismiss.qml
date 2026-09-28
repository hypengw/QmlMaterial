pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.Control {
    id: control
    default property alias content: m_foreground.data
    readonly property MD.SwipeToDismissState dismissState: MD.SwipeToDismissState {
        id: m_state
        distance: control.availableWidth
    }
    property alias startToEndEnabled: m_state.startToEndEnabled
    property alias endToStartEnabled: m_state.endToStartEnabled
    property alias positionalThreshold: m_state.positionalThreshold
    property alias velocityThreshold: m_state.velocityThreshold
    property bool gesturesEnabled: true
    property bool animationsEnabled: true
    readonly property real presentedOffset: __offset * (mirrored ? -1 : 1)
    signal dismissed(int direction)

    property real __offset: 0
    property int __revision: 0
    property bool __complete: false
    property bool __dragAccepted: false

    function __cancelDrag() {
        __dragAccepted = false;
        m_state.cancel();
    }
    function __logicalDelta(vector) {
        const start = control.mapFromItem(null, 0, 0);
        const end = control.mapFromItem(null, vector.x, vector.y);
        return (end.x - start.x) * (control.mirrored ? -1 : 1);
    }
    function __present(revision, offset, animate) {
        m_animation.stop();
        __revision = revision;
        if (__complete && animationsEnabled && animate && __offset !== offset) {
            m_animation.from = __offset;
            m_animation.to = offset;
            m_animation.start();
        } else {
            __offset = offset;
            m_state.complete(revision);
        }
    }
    onAnimationsEnabledChanged: {
        if (!animationsEnabled && m_animation.running) {
            m_animation.stop();
            __offset = m_state.offset;
            m_state.complete(__revision);
        }
    }
    onMirroredChanged: __cancelDrag()
    onVisibleChanged: {
        if (!visible)
            __cancelDrag();
    }
    Component.onCompleted: {
        __offset = m_state.offset;
        __complete = true;
    }
    Connections {
        target: m_state
        function onMotionChanged() {
            if (m_state.dragging) {
                m_animation.stop();
                control.__offset = m_state.offset;
            }
        }
        function onTransitionRequested(revision, offset, animate) {
            control.__present(revision, offset, animate);
        }
        function onDismissed(direction) {
            control.dismissed(direction);
        }
    }
    NumberAnimation {
        id: m_animation
        target: control
        property: "__offset"
        duration: MD.Token.duration.medium2
        easing: MD.Token.easing.standard
        onFinished: m_state.complete(control.__revision)
    }
    DragHandler {
        id: m_drag
        target: null
        enabled: control.enabled && control.visible && control.Window.window !== null && control.Window.window.visible && control.gesturesEnabled && m_state.settledValue === MD.SwipeToDismissState.Settled && (control.startToEndEnabled || control.endToStartEnabled)
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
                control.__dragAccepted = m_state.begin(control.__offset);
        }
        onTranslationChanged: delta => {
            if (active && control.__dragAccepted)
                m_state.dragBy(control.__logicalDelta(delta));
        }
        onCanceled: control.__cancelDrag()
        onGrabChanged: (transition, point) => {
            if (transition === PointerDevice.UngrabExclusive && control.__dragAccepted) {
                control.__dragAccepted = false;
                m_state.release(control.__logicalDelta(point.velocity));
            }
        }
    }
    contentItem: Item {
        id: m_foreground
        implicitWidth: childrenRect.x + childrenRect.width
        implicitHeight: childrenRect.y + childrenRect.height
        transform: Translate {
            x: control.presentedOffset
        }
    }
}
