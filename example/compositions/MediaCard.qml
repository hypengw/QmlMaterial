pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Control {
    id: root
    implicitWidth: 560
    implicitHeight: expanded ? 184 : 128
    padding: 0
    property bool expanded: true
    property bool playing: false
    property bool animationsEnabled: true
    property bool seekAvailable: true
    property int trackIndex: 0
    property real position: 52
    readonly property var tracks: [
        {
            title: 'Daylight',
            artist: 'Paper Coast',
            duration: 303,
            artwork: Qt.resolvedUrl('../assets/carousel/abstract-coral-landscape.jpg')
        },
        {
            title: 'Blue hour',
            artist: 'Quiet Signals',
            duration: 248,
            artwork: Qt.resolvedUrl('../assets/carousel/abstract-indigo-portrait.jpg')
        },
        {
            title: 'Open water',
            artist: 'Soft Current',
            duration: 276,
            artwork: Qt.resolvedUrl('../assets/carousel/abstract-mint-square.jpg')
        }
    ]
    readonly property var track: tracks[trackIndex]
    property string outputDevice: 'Headphones'
    function skip(offset) {
        trackIndex = (trackIndex + offset + tracks.length) % tracks.length;
    }
    function closeOutputs() {
        outputs.close();
    }
    onTrackIndexChanged: position = 0
    function timeText(seconds) {
        const whole = Math.max(0, Math.floor(seconds));
        return Math.floor(whole / 60) + ':' + String(whole % 60).padStart(2, '0');
    }

    background: Rectangle {
        radius: 28
        color: root.MD.MProp.color.surface_container
        MD.Image {
            objectName: 'mediaArtwork'
            anchors.fill: parent
            sourceSize: Qt.size(768, 384)
            radius: 28
            source: root.track.artwork
            opacity: 0.18
            Accessible.role: Accessible.Graphic
            Accessible.name: root.track.title + ' artwork'
        }
    }
    contentItem: Item {
        id: content
        MD.Icon {
            x: 16
            y: 16
            size: 24
            name: MD.Token.icon.music_note
            color: root.MD.MProp.color.on_surface
        }
        MD.Button {
            id: output
            objectName: 'mediaOutput'
            x: parent.width - width - 16
            y: 16
            width: Math.min(implicitWidth, parent.width * 0.5)
            height: 32
            topInset: 0
            bottomInset: 0
            leftInset: 0
            rightInset: 0
            padding: 8
            leftPadding: 8
            rightPadding: 8
            spacing: 4
            mdState.type: MD.Enum.BtFilledTonal
            mdState.corners: MD.Util.corners(16)
            typescale: MD.Token.typescale.label_medium
            text: root.outputDevice
            icon.name: MD.Token.icon.headphones
            icon.width: 16
            icon.height: 16
            Accessible.name: 'Audio output'
            onClicked: outputs.open()
        }
        Column {
            id: metadata
            x: 16
            y: root.height - (root.expanded ? 64 : 16) - implicitHeight
            width: Math.max(0, parent.width - (root.expanded ? 96 : 176))
            spacing: 0
            MD.Text {
                objectName: 'mediaTitle'
                width: parent.width
                text: root.track.title
                typescale: MD.Token.typescale.title_medium
                wrapMode: Text.NoWrap
            }
            MD.Text {
                width: parent.width
                text: root.track.artist
                color: root.MD.MProp.color.on_surface_variant
                typescale: MD.Token.typescale.body_medium
                wrapMode: Text.NoWrap
            }
        }
        Item {
            id: timeline
            objectName: 'mediaTimeline'
            x: root.expanded ? 0 : parent.width - 160
            y: parent.height - (root.expanded ? 48 : 64)
            width: root.expanded ? parent.width : 96
            height: 48
            readonly property bool showTime: root.expanded && seek.seeking
            MD.IconButton {
                objectName: 'previousTrack'
                visible: !timeline.showTime
                x: 0
                width: 48
                height: 48
                padding: 12
                topInset: 0
                bottomInset: 0
                leftInset: 0
                rightInset: 0
                icon.name: MD.Token.icon.skip_previous
                Accessible.name: 'Previous track'
                MD.ToolTip.text: 'Previous track'
                MD.ToolTip.visible: mdState.interactions.hovered
                onClicked: root.skip(-1)
            }
            MD.Text {
                objectName: 'mediaElapsed'
                x: 0
                width: 48
                height: parent.height
                visible: timeline.showTime
                text: root.timeText(seek.previewPosition)
                typescale: MD.Token.typescale.body_small
                color: root.MD.MProp.color.on_surface_variant
                wrapMode: Text.NoWrap
            }
            MediaSeekBar {
                id: seek
                objectName: 'mediaSeek'
                visible: root.expanded
                x: 48
                width: Math.max(0, parent.width - 96)
                height: parent.height
                from: 0
                to: root.track.duration
                playbackPosition: root.position
                playing: root.playing
                animationsEnabled: root.animationsEnabled
                seekAvailable: root.seekAvailable
                onSeekRequested: function (position) {
                    root.position = position;
                }
            }
            MD.Text {
                objectName: 'mediaDuration'
                x: parent.width - width
                width: 48
                height: parent.height
                visible: timeline.showTime
                text: root.timeText(root.track.duration)
                typescale: MD.Token.typescale.body_small
                color: root.MD.MProp.color.on_surface_variant
                horizontalAlignment: Text.AlignRight
                wrapMode: Text.NoWrap
            }
            MD.IconButton {
                objectName: 'nextTrack'
                visible: !timeline.showTime
                x: parent.width - width
                width: 48
                height: 48
                padding: 12
                topInset: 0
                bottomInset: 0
                leftInset: 0
                rightInset: 0
                icon.name: MD.Token.icon.skip_next
                Accessible.name: 'Next track'
                MD.ToolTip.text: 'Next track'
                MD.ToolTip.visible: mdState.interactions.hovered
                onClicked: root.skip(1)
            }
        }
        Item {
            objectName: 'mediaControls'
            x: parent.width - width - 16
            y: parent.height - (root.expanded ? 112 : 64)
            width: 48
            height: 48
            MD.IconToggleButton {
                id: play
                objectName: 'playPause'
                width: 48
                height: 48
                padding: 12
                topInset: 0
                bottomInset: 0
                leftInset: 0
                rightInset: 0
                icon.name: root.playing ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                mdState.type: MD.Enum.IBtFilled
                mdState.elevation: 0
                mdState.corners: MD.Util.corners(root.playing ? 16 : 24)
                mdState.backgroundColor: root.playing ? root.MD.MProp.color.primary : root.MD.MProp.color.secondary_container
                mdState.textColor: root.playing ? root.MD.MProp.color.on_primary : root.MD.MProp.color.on_secondary_container
                checked: root.playing
                onCheckedChangeRequested: value => root.playing = value
                Accessible.name: root.playing ? 'Pause' : 'Play'
                MD.ToolTip.text: root.playing ? 'Pause' : 'Play'
                MD.ToolTip.visible: mdState.interactions.hovered
            }
        }
    }
    MD.Popup {
        id: outputs
        objectName: 'mediaOutputs'
        anchorItem: output
        width: Math.min(280, root.width)
        padding: 8
        dim: false
        modal: true
        contentItem: Column {
            Repeater {
                model: ['Headphones', 'This device', 'Desk speaker']
                delegate: MD.ListItem {
                    required property string modelData
                    width: parent.width
                    text: modelData
                    highlighted: root.outputDevice === modelData
                    icon.name: MD.Token.icon.speaker
                    onClicked: {
                        root.outputDevice = modelData;
                        outputs.close();
                    }
                }
            }
        }
    }
}
