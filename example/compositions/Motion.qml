pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 800
    implicitHeight: content.implicitHeight

    property int presetIndex: 0
    property bool animationsEnabled: true
    property bool atEnd: false
    property bool _ready: false
    readonly property var spatialPreset: {
        const presets = MD.Token.motion.expressive;
        switch (presetIndex) {
        case 1:
            return presets.defaultSpatial;
        case 2:
            return presets.slowSpatial;
        default:
            return presets.fastSpatial;
        }
    }
    readonly property bool running: spatialAnimation.running
    property real progress: 0

    function stop() {
        spatialAnimation.stop();
    }
    function synchronize() {
        progress = atEnd ? 1 : 0;
    }
    function toggleTarget() {
        atEnd = !atEnd;
        animateToTarget();
    }
    function animateToTarget() {
        stop();
        if (!animationsEnabled) {
            synchronize();
            return;
        }
        spatialAnimation.from = progress;
        spatialAnimation.to = atEnd ? 1 : 0;
        spatialAnimation.start();
    }
    function reset() {
        stop();
        atEnd = false;
        synchronize();
    }
    function finish() {
        spatialAnimation.complete();
    }
    onPresetIndexChanged: if (_ready && running)
        animateToTarget()
    onAnimationsEnabledChanged: if (_ready && !animationsEnabled)
        finish()
    onVisibleChanged: if (_ready && !visible)
        finish()
    Component.onCompleted: {
        _ready = true;
        reset();
    }

    NumberAnimation {
        id: spatialAnimation
        objectName: 'spatialAnimation'
        target: root
        property: 'progress'
        duration: root.spatialPreset.duration
        easing: root.spatialPreset.easing
    }

    Column {
        id: content
        width: parent.width
        spacing: 20

        MD.ComboBox {
            objectName: 'presetSelector'
            width: Math.min(280, content.width)
            model: ['Fast spatial', 'Default spatial', 'Slow spatial']
            currentIndex: root.presetIndex
            onActivated: root.presetIndex = currentIndex
        }

        Flow {
            objectName: 'motionControls'
            width: parent.width
            spacing: 8
            MD.IconButton {
                objectName: 'playButton'
                icon.name: root.atEnd ? MD.Token.icon.undo : MD.Token.icon.play_arrow
                MD.ToolTip.text: root.atEnd ? 'Reverse' : 'Play'
                MD.ToolTip.visible: hovered
                Accessible.name: root.atEnd ? 'Reverse' : 'Play'
                onClicked: root.toggleTarget()
            }
            MD.IconButton {
                icon.name: MD.Token.icon.restart_alt
                MD.ToolTip.text: 'Reset'
                MD.ToolTip.visible: hovered
                Accessible.name: 'Reset'
                onClicked: root.reset()
            }
            MD.IconButton {
                icon.name: MD.Token.icon.skip_next
                enabled: root.running
                MD.ToolTip.text: 'Complete'
                MD.ToolTip.visible: hovered
                Accessible.name: 'Complete'
                onClicked: root.finish()
            }
            Row {
                height: 48
                spacing: 8
                MD.Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: root.animationsEnabled
                    onToggled: root.animationsEnabled = checked
                    Accessible.name: 'Animation'
                }
                MD.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: 'Animation'
                }
            }
        }

        Row {
            width: parent.width
            spacing: 12
            MD.Label {
                text: 'Spatial'
                typescale: MD.Token.typescale.title_medium
            }
            MD.Label {
                text: root.spatialPreset.duration + ' ms'
                typescale: MD.Token.typescale.body_medium
            }
        }
        Item {
            id: spatialBand
            width: parent.width
            height: 144
            readonly property real travel: Math.max(0, width - marker.width) * 0.7
            readonly property real start: (width - marker.width - travel) / 2
            Rectangle {
                x: spatialBand.start + marker.width / 2
                y: parent.height / 2
                width: spatialBand.travel
                height: 1
                color: root.MD.MProp.color.outline_variant
            }
            Rectangle {
                id: marker
                objectName: 'spatialMarker'
                width: Math.min(48, parent.width)
                height: width
                x: spatialBand.start + root.progress * spatialBand.travel
                y: (parent.height - height) / 2
                radius: Math.max(0, Math.min(width / 2, 8 + root.progress * 16))
                color: root.MD.MProp.color.primary
                rotation: root.progress * 90
                MD.Icon {
                    anchors.centerIn: parent
                    size: 24
                    name: MD.Token.icon.star
                    color: root.MD.MProp.color.on_primary
                }
            }
        }
    }
}
