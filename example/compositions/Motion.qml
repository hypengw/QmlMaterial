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
    readonly property var spatialPreset: {
        const presets = MD.Token.motion.spring.expressive;
        switch (presetIndex) {
        case 1:
            return presets.defaultSpatial;
        case 2:
            return presets.slowSpatial;
        default:
            return presets.fastSpatial;
        }
    }
    readonly property bool running: spatialMotion.running
    readonly property real progress: spatialMotion.value

    function stop() {
        spatialMotion.stop();
    }
    function synchronize() {
        spatialMotion.snapTo(atEnd ? 1 : 0);
    }
    function toggleTarget() {
        atEnd = !atEnd;
    }
    function reset() {
        stop();
        atEnd = false;
        synchronize();
    }
    function finish() {
        spatialMotion.complete();
    }
    MD.SpringMotion {
        id: spatialMotion
        objectName: 'spatialMotion'
        enabled: root.animationsEnabled && root.visible && (!root.Window.window || (root.Window.window.visible && root.Window.window.visibility !== Window.Minimized))
        targetValue: root.atEnd ? 1 : 0
        spec: root.spatialPreset
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
            onSelectionChangeRequested: (index, value) => root.presetIndex = index
        }

        Flow {
            objectName: 'motionControls'
            width: parent.width
            spacing: 8
            MD.IconButton {
                objectName: 'playButton'
                icon.name: root.atEnd ? MD.Token.icon.undo : MD.Token.icon.play_arrow
                MD.ToolTip.text: root.atEnd ? 'Reverse' : 'Play'
                MD.ToolTip.visible: mdState.interactions.hovered
                Accessible.name: root.atEnd ? 'Reverse' : 'Play'
                onClicked: root.toggleTarget()
            }
            MD.IconButton {
                icon.name: MD.Token.icon.restart_alt
                MD.ToolTip.text: 'Reset'
                MD.ToolTip.visible: mdState.interactions.hovered
                Accessible.name: 'Reset'
                onClicked: root.reset()
            }
            MD.IconButton {
                icon.name: MD.Token.icon.skip_next
                enabled: root.running
                MD.ToolTip.text: 'Complete'
                MD.ToolTip.visible: mdState.interactions.hovered
                Accessible.name: 'Complete'
                onClicked: root.finish()
            }
            Row {
                height: 48
                spacing: 8
                MD.Switch {
                    anchors.verticalCenter: parent.verticalCenter
                    checked: root.animationsEnabled
                    onCheckedChangeRequested: value => root.animationsEnabled = value
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
                text: 'k ' + root.spatialPreset.stiffness + ' / damping ' + root.spatialPreset.dampingRatio
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
