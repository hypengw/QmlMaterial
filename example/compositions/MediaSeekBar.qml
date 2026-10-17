pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.SliderBase {
    id: root
    property bool playing: false
    property bool animationsEnabled: true
    property bool seekAvailable: true
    property bool listening: true
    property real playbackPosition: 0
    readonly property real previewPosition: valueAt(position)
    signal seekRequested(real position)
    implicitWidth: 180
    implicitHeight: 48
    stepSize: 1
    padding: 0
    layoutDirection: Qt.LeftToRight
    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true
    enabled: seekAvailable
    live: false
    value: playbackPosition
    onMoved: if (!pressed && value !== playbackPosition)
        seekRequested(value)
    onPressedChanged: if (!pressed && value !== playbackPosition)
        seekRequested(value)
    Accessible.name: 'Playback position'

    handle: Rectangle {
        width: 4
        height: 16
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: (root.height - height) / 2
        radius: width / 2
        color: root.MD.MProp.color.primary
        visible: root.seekAvailable
    }
    background: MD.SquigglyProgress {
        objectName: 'mediaWave'
        Accessible.ignored: true
        progress: root.position
        animate: root.playing && !root.pressed && root.listening
        animationsEnabled: root.animationsEnabled
        transitionEnabled: !root.seekAvailable
        color: root.MD.MProp.color.primary
    }
    MD.FocusIndicator {
        corners: MD.Util.corners(8)
        active: root.visualFocus
        inset: true
    }
}
