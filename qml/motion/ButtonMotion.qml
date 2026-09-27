import QtQuick
import Qcm.Material as MD

QtObject {
    id: root

    required property MD.CommonState source
    property bool enabled: false
    property bool _synchronizing: true
    property string _previousState: ""
    readonly property bool _animate: enabled && !_synchronizing
    property Connections _sourceConnection: Connections {
        target: root.source
        function onTargetChanged() {
            root.synchronize();
        }
        function onStateChanged() {
            root._previousState = root.source.state;
        }
    }
    onSourceChanged: synchronize()

    function synchronize() {
        _synchronizing = true;
        Qt.callLater(finishSynchronization);
    }
    function finishSynchronization() {
        _previousState = source.state;
        _synchronizing = false;
    }
    property var duration: MD.Token.duration
    property var easing: MD.Token.easing

    property color textColor: source.textColor
    property color backgroundColor: source.backgroundColor
    property real contentOpacity: source.contentOpacity
    property real backgroundOpacity: source.backgroundOpacity
    property MD.corners corners: source.corners

    function colorDuration() {
        if (source.state === "pressed")
            return duration.short1;
        if (_previousState === "pressed" && source.state === "hovered")
            return duration.short2;
        return duration.short4;
    }

    Behavior on textColor {
        enabled: root._animate
        onTargetValueChanged: textAnimation.duration = root.colorDuration()
        ColorAnimation {
            id: textAnimation
            easing: root.easing.linear
        }
    }
    Behavior on backgroundColor {
        enabled: root._animate
        onTargetValueChanged: backgroundAnimation.duration = root.colorDuration()
        ColorAnimation {
            id: backgroundAnimation
            easing: root.easing.linear
        }
    }
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
    Behavior on corners {
        enabled: root._animate
        PropertyAnimation {
            duration: root.duration.short2
            easing: root.easing.linear
        }
    }
}
