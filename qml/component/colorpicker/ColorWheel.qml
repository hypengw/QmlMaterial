pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root

    property real hue: 0
    property real saturation: 1
    property real value: 1
    property real thickness: 22

    signal userHueChanged(real h)
    signal userSvChanged(real s, real v)

    readonly property real _cx: width / 2
    readonly property real _cy: height / 2
    readonly property real _outerR: Math.min(width, height) / 2 - 1
    readonly property real _innerR: _outerR - thickness
    readonly property real _midR: (_outerR + _innerR) / 2

    implicitWidth: 240
    implicitHeight: 240

    function _angleFromXY(x, y) {
        let a = Math.atan2(y - root._cy, x - root._cx) * 180 / Math.PI;
        if (a < 0) a += 360;
        return a;
    }
    function _distFromCenter(x, y) {
        const dx = x - root._cx;
        const dy = y - root._cy;
        return Math.sqrt(dx * dx + dy * dy);
    }

    // Hue ring as an annulus filled with conical gradient.
    MD.Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillRule: ShapePath.OddEvenFill
            startX: root._cx + root._outerR
            startY: root._cy

            PathAngleArc {
                centerX: root._cx; centerY: root._cy
                radiusX: root._outerR; radiusY: root._outerR
                startAngle: 0
                sweepAngle: 360
            }
            PathMove { x: root._cx + root._innerR; y: root._cy }
            PathAngleArc {
                centerX: root._cx; centerY: root._cy
                radiusX: root._innerR; radiusY: root._innerR
                startAngle: 0
                sweepAngle: 360
            }

            fillGradient: ConicalGradient {
                centerX: root._cx
                centerY: root._cy
                angle: 0
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

    // Hue indicator: radial line across the ring (not a circle).
    Item {
        readonly property real _ang: root.hue * 2 * Math.PI
        x: root._cx + root._midR * Math.cos(_ang) - width / 2
        y: root._cy + root._midR * Math.sin(_ang) - height / 2
        width: root.thickness + 8
        height: 4
        rotation: root.hue * 360

        Rectangle {
            anchors.fill: parent
            radius: 2
            color: "white"
            border.color: "black"
            border.width: 1
        }
    }

    // Inscribed SV triangle — vertices touch the inner edge of the ring.
    MD.ColorTriangle {
        id: m_tri
        anchors.centerIn: parent
        width: 2 * root._innerR
        height: 2 * root._innerR
        hue: root.hue
        saturation: root.saturation
        value: root.value
        onUserSvChanged: function (s, v) { root.userSvChanged(s, v); }
    }

    MouseArea {
        id: m_ringArea
        anchors.fill: parent
        property bool dragging: false
        cursorShape: Qt.PointingHandCursor

        onPressed: function (e) {
            const d = root._distFromCenter(e.x, e.y);
            if (d >= root._innerR && d <= root._outerR + 2) {
                m_ringArea.dragging = true;
                const a = root._angleFromXY(e.x, e.y);
                root.userHueChanged(a / 360);
                e.accepted = true;
            } else {
                e.accepted = false;
            }
        }
        onPositionChanged: function (e) {
            if (!m_ringArea.dragging) return;
            const a = root._angleFromXY(e.x, e.y);
            root.userHueChanged(a / 360);
        }
        onReleased: m_ringArea.dragging = false
        onCanceled: m_ringArea.dragging = false
    }
}
