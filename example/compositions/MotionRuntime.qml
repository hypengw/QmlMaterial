pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Column {
    id: root
    spacing: 16
    width: 600
    property string interaction: ""

    MD.Animatable {
        id: offset
        objectName: "offset"
        spec: MD.Motion.spring(200, 0.8)
    }
    MD.AnimatedColor {
        id: tint
        targetValue: offset.active ? "#8c63a8" : "#4d8476"
    }
    MD.Label {
        text: "Interruptible value"
    }
    Rectangle {
        width: parent.width
        height: 64
        color: "transparent"
        Rectangle {
            x: offset.value * Math.max(0, parent.width - width)
            width: 56
            height: 56
            radius: 16
            color: tint.value
        }
    }
    Flow {
        width: parent.width
        spacing: 8
        MD.Button {
            text: "Animate"
            onClicked: offset.animateTo(offset.targetValue > 0.5 ? 0 : 1)
        }
        MD.Button {
            text: offset.paused ? "Resume" : "Pause"
            onClicked: offset.paused ? offset.resume() : offset.pause()
        }
        MD.Button {
            text: "Stop"
            onClicked: offset.stop()
        }
        MD.Button {
            text: "Complete"
            onClicked: offset.complete()
        }
    }
    MD.SeekableTransition {
        id: timeline
        objectName: "timeline"
        MD.RealTrack {
            id: distance
            values: ({
                    from: 0,
                    to: 1
                })
            spec: MD.Motion.tween(800)
        }
        MD.RealTrack {
            id: alpha
            values: ({
                    from: 0.3,
                    to: 1
                })
            spec: MD.Motion.tween(300)
        }
    }
    MD.Label {
        text: "Seekable transition"
    }
    Rectangle {
        width: parent.width
        height: 64
        color: "transparent"
        Rectangle {
            x: distance.value * Math.max(0, parent.width - width)
            width: 56
            height: 56
            radius: 16
            opacity: alpha.value
            color: "#707ca9"
        }
    }
    Flow {
        width: parent.width
        spacing: 8
        MD.Button {
            text: "Seek 70%"
            onClicked: {
                if (!root.interaction)
                    root.interaction = timeline.beginSeek("to");
                timeline.seek(root.interaction, 0.7);
            }
        }
        MD.Button {
            text: "Seek 20%"
            enabled: !!root.interaction
            onClicked: timeline.seek(root.interaction, 0.2)
        }
        MD.Button {
            text: "Cancel"
            enabled: !!root.interaction
            onClicked: {
                timeline.settle(root.interaction, MD.SeekableTransition.Origin, -1, MD.Motion.spring());
                root.interaction = "";
            }
        }
        MD.Button {
            text: "Commit"
            enabled: !!root.interaction
            onClicked: {
                timeline.settle(root.interaction, MD.SeekableTransition.Destination, 1, MD.Motion.spring());
                root.interaction = "";
            }
        }
        MD.Button {
            text: "Reset"
            onClicked: {
                timeline.snapTo("from");
                root.interaction = "";
            }
        }
    }
}
