pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root

    property real value: 0
    property real from: 0
    property real to: 1

    signal moved(real v)

    implicitHeight: 20
    implicitWidth: 220

    readonly property real _trackLeft: 0
    readonly property real _trackRight: width
    readonly property real _trackWidth: _trackRight - _trackLeft

    function _toX(v) {
        const t = (v - from) / (to - from);
        return _trackLeft + Math.max(0, Math.min(1, t)) * _trackWidth;
    }
    function _fromX(x) {
        const t = Math.max(0, Math.min(1, (x - _trackLeft) / _trackWidth));
        return from + t * (to - from);
    }

    MD.Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            startX: 0; startY: 0
            PathLine { x: root.width; y: 0 }
            PathLine { x: root.width; y: root.height }
            PathLine { x: 0; y: root.height }
            PathLine { x: 0; y: 0 }
            fillGradient: LinearGradient {
                x1: 0; y1: 0; x2: root.width; y2: 0
                GradientStop { position: 0.0000; color: "#ff0000" }
                GradientStop { position: 0.1667; color: "#ffff00" }
                GradientStop { position: 0.3333; color: "#00ff00" }
                GradientStop { position: 0.5000; color: "#00ffff" }
                GradientStop { position: 0.6667; color: "#0000ff" }
                GradientStop { position: 0.8333; color: "#ff00ff" }
                GradientStop { position: 1.0000; color: "#ff0000" }
            }
        }
    }

    Rectangle {
        x: root._toX(root.value) - width / 2
        y: -2
        width: 4
        height: root.height + 4
        color: Qt.hsva(root.value, 1, 1, 1)
        border.width: 2
        border.color: "white"
    }
    Rectangle {
        x: root._toX(root.value) - width / 2
        y: -2
        width: 4
        height: root.height + 4
        color: "transparent"
        border.width: 1
        border.color: MD.Util.transparent("black", 0.4)
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onPressed: function (e) { root.moved(root._fromX(e.x)); }
        onPositionChanged: function (e) { if (pressed) root.moved(root._fromX(e.x)); }
    }
}
