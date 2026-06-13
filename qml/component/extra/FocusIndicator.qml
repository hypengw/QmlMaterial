import QtQuick
import Qcm.Material as MD

// Material 3 dual-stroke keyboard-focus ring. Drawn outside the parent,
// driven by `active` (typically bound to control.visualFocus). Not
// hit-testable.
Item {
    id: root
    enabled: false
    z: 10

    property MD.corners corners: MD.Util.corners(0)
    property color outerColor: MD.MProp.color.secondary
    property color innerColor: MD.MProp.color.on_secondary
    property bool  active: false

    readonly property real _outer: MD.Token.state.focus_ring.outer_stroke_width
    readonly property real _inner: MD.Token.state.focus_ring.inner_stroke_width
    readonly property real _inset: MD.Token.state.focus_ring.inner_stroke_inset
    readonly property real _gap:   MD.Token.state.focus_ring.outer_offset

    readonly property real _outerR: root._gap + root._outer / 2
    readonly property real _innerR: root._gap + root._inset + root._inner / 2

    anchors.fill: parent
    anchors.margins: -(_gap + _outer)

    opacity: active ? 1 : 0
    visible: opacity > 0
    Behavior on opacity {
        NumberAnimation {
            duration: MD.Token.duration.short2
            easing.type: MD.Token.easing.standard.type
        }
    }

    // Draw inner first so the outer stroke keeps its full width over the overlap.
    MD.Rectangle {
        anchors.fill: parent
        anchors.margins: root._outer - root._inset
        color: "transparent"
        antialiasing: true
        border.width: root._inner
        border.color: root.innerColor
        corners: MD.Util.corners(
            root.corners.topLeft     + root._innerR,
            root.corners.topRight    + root._innerR,
            root.corners.bottomLeft  + root._innerR,
            root.corners.bottomRight + root._innerR)
    }

    MD.Rectangle {
        anchors.fill: parent
        color: "transparent"
        antialiasing: true
        border.width: root._outer
        border.color: root.outerColor
        corners: MD.Util.corners(
            root.corners.topLeft     + root._outerR,
            root.corners.topRight    + root._outerR,
            root.corners.bottomLeft  + root._outerR,
            root.corners.bottomRight + root._outerR)
    }
}
