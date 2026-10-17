pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

Item {
    id: root
    enum Kind {
        SimpleHour,
        DayHour,
        ScallopHour,
        SimpleMinute,
        ScallopMinute,
        SimpleSecond,
        ScallopSecond
    }
    required property color color
    property int kind: ClockWidgetHand.SimpleHour
    readonly property bool outlined: kind === ClockWidgetHand.DayHour
    readonly property real dialScale: Math.min(width, height) / 380
    readonly property var drawing: {
        switch (kind) {
        case ClockWidgetHand.SimpleHour:
            return {
                width: 29,
                height: 101,
                viewport: Qt.size(18, 63),
                top: 0.187,
                bottom: 0.467,
                path: 'M9,0.5L9,0.5A7,7 0,0 1,16 7.5L16,55.5A7,7 0,0 1,9 62.5L9,62.5A7,7 0,0 1,2 55.5L2,7.5A7,7 0,0 1,9 0.5z',
                detail: ''
            };
        case ClockWidgetHand.DayHour:
            return {
                width: 29,
                height: 111,
                viewport: Qt.size(18, 69),
                top: 0.2,
                bottom: 0.45,
                path: 'M9,3.5L9,3.5A7,7 0,0 1,16 10.5L16,58.5A7,7 0,0 1,9 65.5L9,65.5A7,7 0,0 1,2 58.5L2,10.5A7,7 0,0 1,9 3.5z',
                detail: ''
            };
        case ClockWidgetHand.ScallopHour:
            return {
                width: 38,
                height: 114,
                viewport: Qt.size(38, 114),
                top: 0.25,
                bottom: 0.45,
                path: 'M19,1L19,1A16,16 0,0 1,35 17L35,95A16,16 0,0 1,19 111L19,111A16,16 0,0 1,3 95L3,17A16,16 0,0 1,19 1z',
                detail: ''
            };
        case ClockWidgetHand.SimpleMinute:
            return {
                width: 6,
                height: 137,
                viewport: Qt.size(4, 85),
                top: 0.044,
                bottom: 0.489,
                path: 'M1.867,0.5L2.133,0.5A1.867,1.867 0,0 1,4 2.367L4,82.633A1.867,1.867 0,0 1,2.133 84.5L1.867,84.5A1.867,1.867 0,0 1,0 82.633L0,2.367A1.867,1.867 0,0 1,1.867 0.5z',
                detail: ''
            };
        case ClockWidgetHand.ScallopMinute:
            return {
                width: 38,
                height: 152,
                viewport: Qt.size(38, 152),
                top: 0.15,
                bottom: 0.45,
                path: 'M19,3L19,3A16,16 0,0 1,35 19L35,133A16,16 0,0 1,19 149L19,149A16,16 0,0 1,3 133L3,19A16,16 0,0 1,19 3z',
                detail: ''
            };
        case ClockWidgetHand.SimpleSecond:
            return {
                width: 10,
                height: 145,
                viewport: Qt.size(6, 87),
                top: 0.0385,
                bottom: 0.4835,
                path: 'm3,81c-1.657,0 -3,1.343 -3,3 0,1.657 1.343,3 3,3 1.657,0 3,-1.343 3,-3 0,-1.657 -1.343,-3 -3,-3z',
                detail: 'M3,84L3,84A0.467,0.467 90.225,0 1,2.533 83.533L2.533,1.4A0.467,0.467 90.225,0 1,3 0.933L3,0.933A0.467,0.467 90.225,0 1,3.467 1.4L3.467,83.533A0.467,0.467 90.225,0 1,3 84z'
            };
        case ClockWidgetHand.ScallopSecond:
            return {
                width: 38,
                height: 38,
                viewport: Qt.size(38, 38),
                top: 0.05,
                bottom: 0.85,
                path: 'M19,1C28.941,1 37,9.059 37,19 37,28.941 28.941,37 19,37 9.059,37 1,28.941 1,19 1,9.059 9.059,1 19,1Z',
                detail: ''
            };
        default:
            return null;
        }
    }
    implicitWidth: 380
    implicitHeight: 380

    MD.Shape {
        id: geometry
        objectName: 'handGeometry'
        visible: root.drawing !== null
        width: root.drawing?.viewport.width ?? 0
        height: root.drawing?.viewport.height ?? 0
        x: (root.width - (root.drawing?.width ?? 0) * root.dialScale) / 2
        // InsetDrawable positions its intrinsic rectangle around the clock pivot.
        y: root.height / 2 + (root.drawing ? root.drawing.height * (root.drawing.top - 0.5) / (1 - root.drawing.top - root.drawing.bottom) : 0) * root.dialScale
        transform: Scale {
            xScale: (root.drawing?.width ?? 0) * root.dialScale / Math.max(1, geometry.width)
            yScale: (root.drawing?.height ?? 0) * root.dialScale / Math.max(1, geometry.height)
        }
        ShapePath {
            objectName: 'handOutline'
            strokeColor: root.outlined ? root.color : 'transparent'
            strokeWidth: root.outlined ? 3 : 0
            fillColor: root.outlined ? 'transparent' : root.color
            fillRule: root.kind === ClockWidgetHand.SimpleSecond ? ShapePath.OddEvenFill : ShapePath.WindingFill
            PathSvg {
                path: root.drawing?.path ?? ''
            }
        }
        ShapePath {
            strokeWidth: 0
            fillColor: root.color
            PathSvg {
                path: root.drawing?.detail ?? ''
            }
        }
    }
}
