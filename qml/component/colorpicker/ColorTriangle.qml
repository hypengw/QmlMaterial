pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root

    property real hue: 0
    property real saturation: 1
    property real value: 1

    signal userSvChanged(real s, real v)

    implicitWidth: 180
    implicitHeight: 180

    readonly property real _cx: width / 2
    readonly property real _cy: height / 2
    readonly property real _r: Math.min(width, height) / 2

    // V_H aligned with current hue position on the ring.
    readonly property real _hAng: hue * 2 * Math.PI
    readonly property real _wAng: _hAng + 2 * Math.PI / 3
    readonly property real _kAng: _hAng + 4 * Math.PI / 3

    readonly property real _vhx: _cx + _r * Math.cos(_hAng)
    readonly property real _vhy: _cy + _r * Math.sin(_hAng)
    readonly property real _vwx: _cx + _r * Math.cos(_wAng)
    readonly property real _vwy: _cy + _r * Math.sin(_wAng)
    readonly property real _vkx: _cx + _r * Math.cos(_kAng)
    readonly property real _vky: _cy + _r * Math.sin(_kAng)

    readonly property color _hueColor: Qt.hsva(hue, 1, 1, 1)

    MD.Shape {
        anchors.fill: parent

        ShapePath {
            fillColor: root._hueColor
            strokeColor: root._hueColor
            strokeWidth: 1
            startX: root._vhx; startY: root._vhy
            PathLine { x: root._vwx; y: root._vwy }
            PathLine { x: root._vkx; y: root._vky }
            PathLine { x: root._vhx; y: root._vhy }
        }
        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            startX: root._vhx; startY: root._vhy
            fillGradient: LinearGradient {
                x1: (root._vhx + root._vkx) / 2
                y1: (root._vhy + root._vky) / 2
                x2: root._vwx
                y2: root._vwy
                GradientStop { position: 0.0; color: "#00ffffff" }
                GradientStop { position: 1.0; color: "#ffffffff" }
            }
            PathLine { x: root._vwx; y: root._vwy }
            PathLine { x: root._vkx; y: root._vky }
            PathLine { x: root._vhx; y: root._vhy }
        }
        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            startX: root._vhx; startY: root._vhy
            fillGradient: LinearGradient {
                x1: (root._vhx + root._vwx) / 2
                y1: (root._vhy + root._vwy) / 2
                x2: root._vkx
                y2: root._vky
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#ff000000" }
            }
            PathLine { x: root._vwx; y: root._vwy }
            PathLine { x: root._vkx; y: root._vky }
            PathLine { x: root._vhx; y: root._vhy }
        }
    }

    function _toSV(x, y) {
        const ax = root._vhx, ay = root._vhy;
        const bx = root._vwx, by = root._vwy;
        const cx = root._vkx, cy = root._vky;
        const v0x = bx - ax, v0y = by - ay;
        const v1x = cx - ax, v1y = cy - ay;
        const v2x = x - ax, v2y = y - ay;
        const d00 = v0x * v0x + v0y * v0y;
        const d01 = v0x * v1x + v0y * v1y;
        const d11 = v1x * v1x + v1y * v1y;
        const d20 = v2x * v0x + v2y * v0y;
        const d21 = v2x * v1x + v2y * v1y;
        const denom = d00 * d11 - d01 * d01;
        if (Math.abs(denom) < 1e-9) return null;
        const beta = (d11 * d20 - d01 * d21) / denom;
        const gamma = (d00 * d21 - d01 * d20) / denom;
        const alpha = 1 - beta - gamma;
        const a = MD.Util.clamp(alpha, 0, 1);
        const b = MD.Util.clamp(beta, 0, 1);
        const g = MD.Util.clamp(gamma, 0, 1);
        const sum = a + b + g;
        if (sum < 1e-9) return null;
        const an = a / sum, gn = g / sum;
        const v = 1 - gn;
        const s = v < 1e-6 ? 0 : an / v;
        return { s: MD.Util.clamp(s, 0, 1), v: MD.Util.clamp(v, 0, 1) };
    }
    function _isInside(x, y) {
        const ax = root._vhx, ay = root._vhy;
        const bx = root._vwx, by = root._vwy;
        const cx = root._vkx, cy = root._vky;
        const v0x = bx - ax, v0y = by - ay;
        const v1x = cx - ax, v1y = cy - ay;
        const v2x = x - ax, v2y = y - ay;
        const d00 = v0x * v0x + v0y * v0y;
        const d01 = v0x * v1x + v0y * v1y;
        const d11 = v1x * v1x + v1y * v1y;
        const d20 = v2x * v0x + v2y * v0y;
        const d21 = v2x * v1x + v2y * v1y;
        const denom = d00 * d11 - d01 * d01;
        if (Math.abs(denom) < 1e-9) return false;
        const beta = (d11 * d20 - d01 * d21) / denom;
        const gamma = (d00 * d21 - d01 * d20) / denom;
        const alpha = 1 - beta - gamma;
        return alpha >= 0 && beta >= 0 && gamma >= 0;
    }

    // SV handle
    Rectangle {
        readonly property real _alpha: root.saturation * root.value
        readonly property real _beta: (1 - root.saturation) * root.value
        readonly property real _gamma: 1 - root.value
        readonly property real _px: _alpha * root._vhx + _beta * root._vwx + _gamma * root._vkx
        readonly property real _py: _alpha * root._vhy + _beta * root._vwy + _gamma * root._vky
        x: _px - width / 2
        y: _py - height / 2
        width: 14; height: 14
        radius: 7
        color: "transparent"
        border.color: "white"
        border.width: 2
        Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            radius: width / 2
            color: "transparent"
            border.color: "black"
            border.width: 1
        }
    }

    MouseArea {
        anchors.fill: parent
        property bool dragging: false
        cursorShape: Qt.CrossCursor
        onPressed: function (e) {
            if (root._isInside(e.x, e.y)) {
                dragging = true;
                const sv = root._toSV(e.x, e.y);
                if (sv) root.userSvChanged(sv.s, sv.v);
                e.accepted = true;
            } else {
                e.accepted = false;
            }
        }
        onPositionChanged: function (e) {
            if (!dragging) return;
            const sv = root._toSV(e.x, e.y);
            if (sv) root.userSvChanged(sv.s, sv.v);
        }
        onReleased: dragging = false
        onCanceled: dragging = false
    }
}
