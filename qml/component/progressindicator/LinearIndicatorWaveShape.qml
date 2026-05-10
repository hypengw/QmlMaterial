pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import QtQml.Models
import Qcm.Material as MD

// Wavy variant of LinearIndicatorShape. Active indicators render as wavy
// paths (MD.PathWave); inactive/track segments stay flat per M3 spec.
MD.Shape {
    id: root
    property int strokeWidth: 4

    property color trackColor: root.MD.MProp.color.secondary_container

    property real waveLength: 30
    property real waveAmplitude: 3
    property real phase: 0

    property var indicators: []

    function drawLine(startFraction: real, endFraction: real, gapSize: real): point {
        startFraction = MD.Util.clamp(startFraction, 0, 1);
        endFraction = MD.Util.clamp(endFraction, 0, 1);
        const GAP_RAMP_DOWN_THRESHOLD = 0.01;
        gapSize += strokeWidth / 2;
        const startGapSize = (gapSize * MD.Util.clamp(startFraction, 0, GAP_RAMP_DOWN_THRESHOLD) / GAP_RAMP_DOWN_THRESHOLD);
        const endGapSize = (gapSize * (1 - MD.Util.clamp(endFraction, 1 - GAP_RAMP_DOWN_THRESHOLD, 1)) / GAP_RAMP_DOWN_THRESHOLD);
        const start = width * startFraction + startGapSize;
        const end = width * endFraction - endGapSize;
        if (start >= end) {
            return Qt.point(0, 0);
        }
        return Qt.point(start, end);
    }

    component FlatPath: ShapePath {
        id: m_p
        property point line
        capStyle: ShapePath.RoundCap
        fillColor: "transparent"
        strokeWidth: root.strokeWidth
        startX: line.x
        startY: root.height / 2
        PathLine {
            x: m_p.line.y
            y: root.height / 2
        }
    }

    component WavePath: ShapePath {
        id: m_p
        property point line
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin
        fillColor: "transparent"
        // Hide cleanly when the segment is empty.
        strokeWidth: (line.y - line.x) > 0 ? root.strokeWidth : 0
        startX: line.x
        startY: root.height / 2
        MD.PathWave {
            x: m_p.line.y
            y: root.height / 2
            amplitude: root.waveAmplitude
            waveLength: root.waveLength
            phase: root.phase
        }
    }

    FlatPath {
        line: {
            if (root.indicators.length < 1)
                return Qt.point(0, 0);
            const a = root.indicators[0];
            return root.drawLine(0, a.startFraction, a.gapSize / 2);
        }
        strokeColor: root.trackColor
    }

    FlatPath {
        line: {
            if (root.indicators.length < 1)
                return Qt.point(0, 0);
            const a = root.indicators[root.indicators.length - 1];
            return root.drawLine(a.endFraction, 1, a.gapSize / 2);
        }
        strokeColor: root.trackColor
    }

    Instantiator {
        model: Math.max(root.indicators.length, 1) - 1
        delegate: FlatPath {
            required property int index
            property var cur: root.indicators[index]
            property var next: root.indicators[(index + 1) % Math.max(root.indicators.length, 1)]
            line: cur && next
                  ? root.drawLine(cur.endFraction, next.startFraction, cur.gapSize / 2)
                  : Qt.point(0, 0)
            strokeColor: root.trackColor
        }
        onObjectAdded: (idx, obj) => root.data.push(obj)
        onObjectRemoved: (idx, obj) => MD.Util.removeObj(root.data, obj)
    }

    Instantiator {
        model: root.indicators
        delegate: WavePath {
            required property var modelData
            line: modelData
                  ? root.drawLine(modelData.startFraction, modelData.endFraction, modelData.gapSize / 2)
                  : Qt.point(0, 0)
            strokeColor: modelData ? modelData.color : root.trackColor
        }
        onObjectAdded: (idx, obj) => root.data.push(obj)
        onObjectRemoved: (idx, obj) => MD.Util.removeObj(root.data, obj)
    }
}
