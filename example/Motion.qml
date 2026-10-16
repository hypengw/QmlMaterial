pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Page {
    id: root
    title: 'Motion'
    padding: 0

    property int presetIndex: 0
    property bool animationsEnabled: true
    property bool atEnd: false
    property bool _ready: false
    property color startColor: MD.MProp.color.primary
    property color endColor: MD.MProp.color.tertiary
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
    readonly property var effectsPreset: MD.Token.motion.expressive.defaultEffects
    readonly property bool running: spatialAnimation.running || colorAnimation.running || opacityAnimation.running
    property real progress: 0
    property color effectColor: startColor
    property real effectOpacity: 1

    function stop() {
        spatialAnimation.stop();
        colorAnimation.stop();
        opacityAnimation.stop();
    }
    function synchronize() {
        progress = atEnd ? 1 : 0;
        effectColor = atEnd ? endColor : startColor;
        effectOpacity = atEnd ? 0.35 : 1;
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
        colorAnimation.from = effectColor;
        colorAnimation.to = atEnd ? endColor : startColor;
        opacityAnimation.from = effectOpacity;
        opacityAnimation.to = atEnd ? 0.35 : 1;
        spatialAnimation.start();
        colorAnimation.start();
        opacityAnimation.start();
    }
    function reset() {
        stop();
        atEnd = false;
        synchronize();
    }
    function finish() {
        spatialAnimation.complete();
        colorAnimation.complete();
        opacityAnimation.complete();
    }
    function synchronizeColors() {
        if (_ready) {
            finish();
            synchronize();
        }
    }
    onStartColorChanged: synchronizeColors()
    onEndColorChanged: synchronizeColors()
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
    ColorAnimation {
        id: colorAnimation
        objectName: 'colorAnimation'
        target: root
        property: 'effectColor'
        duration: root.effectsPreset.duration
        easing: root.effectsPreset.easing
    }
    NumberAnimation {
        id: opacityAnimation
        objectName: 'opacityAnimation'
        target: root
        property: 'effectOpacity'
        duration: root.effectsPreset.duration
        easing: root.effectsPreset.easing
    }

    contentItem: MD.VerticalFlickable {
        topMargin: 24
        bottomMargin: 24
        Column {
            id: content
            width: Math.max(0, Math.min(800, parent.width - 32))
            x: (parent.width - width) / 2
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
            MD.Divider {
                width: parent.width
            }
            Row {
                spacing: 12
                MD.Label {
                    text: 'Effects'
                    typescale: MD.Token.typescale.title_medium
                }
                MD.Label {
                    text: root.effectsPreset.duration + ' ms'
                    typescale: MD.Token.typescale.body_medium
                }
            }
            Row {
                width: parent.width
                height: 144
                Item {
                    width: parent.width / 2
                    height: parent.height
                    MD.Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: 'Color'
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.min(72, parent.width)
                        height: width
                        radius: 8
                        color: root.effectColor
                    }
                }
                Item {
                    width: parent.width / 2
                    height: parent.height
                    MD.Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: 'Opacity'
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.min(72, parent.width)
                        height: width
                        radius: 8
                        color: root.MD.MProp.color.tertiary
                        opacity: root.effectOpacity
                    }
                }
            }
        }
    }
}
