import QtQuick
import Qcm.Material as MD

QtObject {
    id: root

    required property MD.CommonState source
    property bool enabled: false
    property bool _synchronizing: true
    readonly property bool _animate: enabled && !_synchronizing
    property Connections _sourceConnection: Connections {
        target: root.source
        function onTargetChanged() {
            root.synchronize();
        }
    }
    onSourceChanged: synchronize()

    function synchronize() {
        _synchronizing = true;
        Qt.callLater(finishSynchronization);
    }
    function finishSynchronization() {
        _synchronizing = false;
    }
    property var duration: MD.Token.duration
    property var easing: MD.Token.easing
    property MD.springSpec shapeSpringSpec: MD.Token.motion.spring.standard.defaultEffects
    property MD.CornerSpringMotion _shapeMotion: MD.CornerSpringMotion {
        enabled: root._animate
        spec: root.shapeSpringSpec
        targetCorners: root.source.corners
    }

    property color textColor: source.textColor
    property color backgroundColor: source.backgroundColor
    property real contentOpacity: source.contentOpacity
    property real backgroundOpacity: source.backgroundOpacity
    property MD.corners corners: _shapeMotion.corners

    Behavior on contentOpacity {
        enabled: root._animate
        NumberAnimation {
            duration: root.duration.short4
            easing: root.easing.linear
        }
    }
    Behavior on backgroundOpacity {
        enabled: root._animate
        NumberAnimation {
            duration: root.duration.short4
            easing: root.easing.linear
        }
    }
}
