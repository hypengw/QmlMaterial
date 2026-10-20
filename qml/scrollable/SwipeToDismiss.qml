pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.Control {
    id: control
    default property alias content: m_foreground.data
    readonly property MD.SwipeToDismissState dismissState: MD.SwipeToDismissState {
        id: m_state
        inputItem: control
        presentedOffset: control.__offset
        distance: control.availableWidth
    }
    property alias startToEndEnabled: m_state.startToEndEnabled
    property alias endToStartEnabled: m_state.endToStartEnabled
    property alias positionalThreshold: m_state.positionalThreshold
    property alias velocityThreshold: m_state.velocityThreshold
    property alias gesturesEnabled: m_state.gesturesEnabled
    property bool animationsEnabled: true
    readonly property real presentedOffset: __offset * (mirrored ? -1 : 1)
    signal dismissed(int direction)

    property real __offset: 0
    property int __revision: 0
    property bool __complete: false

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
    contentItem: Item {
        id: m_foreground
        implicitWidth: childrenRect.x + childrenRect.width
        implicitHeight: childrenRect.y + childrenRect.height
        transform: Translate {
            x: control.presentedOffset
        }
    }
}
