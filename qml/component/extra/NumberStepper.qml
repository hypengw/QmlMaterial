pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root

    property real value: 0
    property real step: 1
    property real from: -Infinity
    property real to: Infinity
    property bool wrap: false

    signal stepped(real value)

    implicitWidth: 18
    implicitHeight: 28

    function _clamp(v) {
        if (root.wrap) {
            if (root.from > -Infinity && root.to < Infinity) {
                const span = root.to - root.from;
                while (v < root.from) v += span;
                while (v > root.to) v -= span;
            }
            return v;
        }
        return Math.max(root.from, Math.min(root.to, v));
    }
    function stepBy(delta) {
        const next = _clamp(root.value + delta);
        root.value = next;
        root.stepped(next);
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Item {
            width: parent.width
            height: parent.height / 2
            Rectangle {
                anchors.fill: parent
                color: m_up.pressed
                    ? MD.Util.transparent(MD.MProp.color.on_surface, 0.12)
                    : m_up.containsMouse
                        ? MD.Util.transparent(MD.MProp.color.on_surface, 0.08)
                        : "transparent"
                radius: 4
            }
            Canvas {
                id: m_upIcon
                anchors.centerIn: parent
                width: 10; height: 6
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    ctx.fillStyle = MD.MProp.color.on_surface_variant;
                    ctx.beginPath();
                    ctx.moveTo(0, height);
                    ctx.lineTo(width / 2, 0);
                    ctx.lineTo(width, height);
                    ctx.closePath();
                    ctx.fill();
                }
                Connections {
                    target: MD.MProp.color
                    function onSchemeChanged() { m_upIcon.requestPaint(); }
                }
            }
            MouseArea {
                id: m_up
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.stepBy(root.step)
                Timer {
                    id: m_upTimer
                    repeat: true
                    interval: 80
                    onTriggered: root.stepBy(root.step)
                }
                onPressed: m_upDelay.start()
                onReleased: { m_upDelay.stop(); m_upTimer.stop(); }
                onCanceled: { m_upDelay.stop(); m_upTimer.stop(); }
                Timer {
                    id: m_upDelay
                    interval: 350
                    onTriggered: m_upTimer.start()
                }
            }
        }
        Item {
            width: parent.width
            height: parent.height / 2
            Rectangle {
                anchors.fill: parent
                color: m_down.pressed
                    ? MD.Util.transparent(MD.MProp.color.on_surface, 0.12)
                    : m_down.containsMouse
                        ? MD.Util.transparent(MD.MProp.color.on_surface, 0.08)
                        : "transparent"
                radius: 4
            }
            Canvas {
                id: m_downIcon
                anchors.centerIn: parent
                width: 10; height: 6
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    ctx.fillStyle = MD.MProp.color.on_surface_variant;
                    ctx.beginPath();
                    ctx.moveTo(0, 0);
                    ctx.lineTo(width / 2, height);
                    ctx.lineTo(width, 0);
                    ctx.closePath();
                    ctx.fill();
                }
                Connections {
                    target: MD.MProp.color
                    function onSchemeChanged() { m_downIcon.requestPaint(); }
                }
            }
            MouseArea {
                id: m_down
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.stepBy(-root.step)
                Timer {
                    id: m_downTimer
                    repeat: true
                    interval: 80
                    onTriggered: root.stepBy(-root.step)
                }
                onPressed: m_downDelay.start()
                onReleased: { m_downDelay.stop(); m_downTimer.stop(); }
                onCanceled: { m_downDelay.stop(); m_downTimer.stop(); }
                Timer {
                    id: m_downDelay
                    interval: 350
                    onTriggered: m_downTimer.start()
                }
            }
        }
    }
}
