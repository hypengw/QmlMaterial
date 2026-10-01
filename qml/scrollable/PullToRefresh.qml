pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import Qcm.Material as MD

MD.Control {
    id: control
    default property alias content: m_content.data
    readonly property MD.PullToRefreshState refreshState: MD.PullToRefreshState {
        id: m_state
        enabled: control.enabled
        settling: m_return.running
    }
    property alias refreshing: m_state.refreshing
    property alias threshold: m_state.threshold
    property bool shapeLoading: false
    property bool animationsEnabled: true
    readonly property real presentedOffset: __offset
    property real __offset: 0
    property bool __complete: false
    signal refreshRequested
    Component {
        id: c_circular_loading
        MD.CircularIndicator {
            implicitWidth: 16
            implicitHeight: 16
            padding: strokeWidth / 2
            strokeWidth: 2.5
            color: MD.MProp.color.on_surface_variant
            running: true
        }
    }
    Component {
        id: c_shape_loading
        MD.BusyIndicator {
            implicitWidth: 24
            implicitHeight: 24
            indicatorSize: 24
            colors: [MD.MProp.color.on_surface_variant]
            running: true
        }
    }
    property Component indicator: MD.ElevationRectangle {
        implicitWidth: 40
        implicitHeight: 40
        radius: width / 2
        color: MD.MProp.color.surface_container_high
        elevation: MD.Token.elevation.level2

        MD.Shape {
            id: m_arrow
            anchors.centerIn: parent
            width: 16
            height: 16
            visible: opacity > 0
            readonly property real progress: control.threshold > 0 ? control.presentedOffset / control.threshold : 0
            readonly property real adjusted: Math.max(0, Math.min(1, progress) - 0.4) * 5 / 3
            readonly property real tension: Math.max(0, Math.min(2, progress - 1))
            readonly property real turn: (-0.25 + 0.4 * adjusted + tension - tension * tension / 4) * 0.5
            readonly property real endAngle: (turn + adjusted * 0.8) * 360
            opacity: control.refreshing ? 0 : progress >= 1 ? 1 : 0.3
            Behavior on opacity {
                enabled: control.__complete && control.animationsEnabled
                NumberAnimation {
                    duration: MD.Token.duration.short4
                    easing: MD.Token.easing.standard
                }
            }
            rotation: turn
            ShapePath {
                fillColor: "transparent"
                strokeColor: MD.MProp.color.on_surface_variant
                strokeWidth: 2.5
                capStyle: ShapePath.FlatCap
                PathAngleArc {
                    centerX: 8
                    centerY: 8
                    radiusX: 6.75
                    radiusY: 6.75
                    startAngle: m_arrow.turn * 360
                    sweepAngle: m_arrow.adjusted * 288
                }
            }
            MD.Shape {
                anchors.fill: parent
                rotation: m_arrow.endAngle - 2.5
                ShapePath {
                    fillColor: "transparent"
                    strokeColor: MD.MProp.color.on_surface_variant
                    strokeWidth: 2.5
                    startX: 14.75 - 5 * m_arrow.adjusted
                    startY: 5.5
                    PathLine {
                        x: 14.75
                        y: 5.5 + 5 * m_arrow.adjusted
                    }
                    PathLine {
                        x: 14.75 + 5 * m_arrow.adjusted
                        y: 5.5
                    }
                }
            }
        }
        Loader {
            anchors.centerIn: parent
            opacity: control.refreshing ? 1 : 0
            active: opacity > 0 && control.visible && control.presentedOffset > 0
            Behavior on opacity {
                enabled: control.__complete && control.animationsEnabled
                NumberAnimation {
                    duration: MD.Token.duration.short4
                    easing: MD.Token.easing.standard
                }
            }
            sourceComponent: control.shapeLoading ? c_shape_loading : c_circular_loading
        }
    }
    function __syncOffset(animate = true) {
        m_return.stop();
        if (__complete && animate && animationsEnabled && !refreshState.dragging) {
            m_return.from = __offset;
            m_return.to = refreshState.offset;
            m_return.start();
        } else {
            __offset = refreshState.offset;
        }
    }
    onAnimationsEnabledChanged: __syncOffset(false)
    Component.onCompleted: {
        __complete = true;
        __syncOffset(false);
    }
    Connections {
        target: m_state
        function onDistanceChanged() {
            control.__syncOffset();
        }
        function onThresholdChanged() {
            control.__syncOffset(false);
        }
        function onRefreshRequested() {
            control.refreshRequested();
        }
    }
    NumberAnimation {
        id: m_return
        target: control
        property: "__offset"
        duration: MD.Token.duration.short4
        easing: MD.Token.easing.standard
    }
    contentItem: Item {
        id: m_content
    }
    property Item __overlay: Item {
        parent: control
        anchors.fill: parent
        clip: true
        z: 1
        Loader {
            x: (parent.width - width) / 2
            y: control.presentedOffset - height
            visible: control.presentedOffset > 0
            sourceComponent: control.indicator
        }
    }
}
