pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.RangeSliderBase {
    id: control

    property MD.StateRangeSlider mdState: MD.StateRangeSlider {
        item: control
    }
    property int sliderSize: MD.Enum.SliderSizeXSmall
    property int labelBehavior: MD.Enum.SliderLabelFloating
    property int tickVisibilityMode: MD.Enum.SliderTickAutoLimit
    property int maxVisibleStops: 20

    implicitWidth: Math.max(horizontal ? 200 : __thickness, first.implicitHandleWidth, second.implicitHandleWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(horizontal ? __thickness : 200, first.implicitHandleHeight, second.implicitHandleHeight) + topPadding + bottomPadding

    readonly property real __thickness: {
        switch (sliderSize) {
        case MD.Enum.SliderSizeSmall:
            return MD.Token.slider.active_track_height_small;
        case MD.Enum.SliderSizeMedium:
            return MD.Token.slider.active_track_height_medium;
        case MD.Enum.SliderSizeLarge:
            return MD.Token.slider.active_track_height_large;
        case MD.Enum.SliderSizeXLarge:
            return MD.Token.slider.active_track_height_xlarge;
        default:
            return MD.Token.slider.active_track_height_xsmall;
        }
    }
    readonly property real __firstCenter: trackStart + first.visualPosition * trackLength
    readonly property real __secondCenter: trackStart + second.visualPosition * trackLength
    readonly property real __lowerCenter: Math.min(__firstCenter, __secondCenter)
    readonly property real __upperCenter: Math.max(__firstCenter, __secondCenter)
    readonly property real __firstGap: MD.Token.slider.thumb_track_gap + (first.handle === firstThumb ? firstThumb.handleLineWidth / 2 : first.handle ? (horizontal ? first.handle.width : first.handle.height) / 2 : 0)
    readonly property real __secondGap: MD.Token.slider.thumb_track_gap + (second.handle === secondThumb ? secondThumb.handleLineWidth / 2 : second.handle ? (horizontal ? second.handle.width : second.handle.height) / 2 : 0)
    readonly property real __lowerGap: __firstCenter <= __secondCenter ? __firstGap : __secondGap
    readonly property real __upperGap: __firstCenter <= __secondCenter ? __secondGap : __firstGap
    readonly property real __trackStart: horizontal ? leftPadding : topPadding
    readonly property real __trackEnd: __trackStart + (horizontal ? availableWidth : availableHeight)
    readonly property var __ticks: {
        if (tickVisibilityMode === MD.Enum.SliderTickNone)
            return [];
        if (stepSize > 0 && from !== to)
            return control.tickPositions(Math.max(2, maxVisibleStops));
        return tickVisibilityMode === MD.Enum.SliderTickAll ? [0, 1] : [];
    }

    component Thumb: MD.SliderHandle {
        id: thumb
        required property int handleIndex
        readonly property var node: handleIndex === 0 ? control.first : control.second
        readonly property MD.StateRangeSlider thumbState: MD.StateRangeSlider {
            item: control
            handleIndex: thumb.handleIndex
        }
        readonly property real center: control.trackStart + node.visualPosition * control.trackLength
        appearance: control.mdState
        horizontal: control.horizontal
        handleWidth: control.mdState.handleWidth
        handleHeight: control.mdState.handleHeight
        handleLineWidth: thumbState.handleLineWidth
        Behavior on handleLineWidth {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.linear
            }
        }
        value: control.valueAt(node.position)
        labelBehavior: control.labelBehavior
        handlePressed: node.pressed
        handleHasFocus: node.focused
        handleHovered: hover.hovered
        opacity: control.mdState.backgroundOpacity
        z: handlePressed || handleHasFocus ? 3 : 2
        x: control.horizontal ? center - width / 2 : control.leftPadding + (control.availableWidth - width) / 2
        y: control.horizontal ? control.topPadding + (control.availableHeight - height) / 2 : center - height / 2
        HoverHandler {
            id: hover
            enabled: control.enabled
        }
    }

    first.handle: Thumb {
        id: firstThumb
        handleIndex: 0
    }
    second.handle: Thumb {
        id: secondThumb
        handleIndex: 1
    }

    background: Item {
        opacity: control.mdState.backgroundOpacity
        Track {
            objectName: "rangeInactiveStart"
            start: control.__trackStart
            end: Math.max(start, control.__lowerCenter - control.__lowerGap)
            outerStart: true
            color: control.mdState.trackInactiveColor
        }
        Track {
            objectName: "rangeActiveTrack"
            start: control.__lowerCenter + control.__lowerGap
            end: control.__upperCenter - control.__upperGap
            color: control.mdState.trackColor
        }
        Track {
            objectName: "rangeInactiveEnd"
            start: Math.min(control.__trackEnd, control.__upperCenter + control.__upperGap)
            end: control.__trackEnd
            outerEnd: true
            color: control.mdState.trackInactiveColor
        }
        Repeater {
            model: control.__ticks
            delegate: Rectangle {
                required property real modelData
                readonly property real visual: control.vertical || control.mirrored ? 1 - modelData : modelData
                readonly property real center: control.trackStart + visual * control.trackLength
                width: MD.Token.slider.stop_indicator_size
                height: width
                radius: width / 2
                x: control.horizontal ? center - width / 2 : control.leftPadding + (control.availableWidth - width) / 2
                y: control.horizontal ? control.topPadding + (control.availableHeight - height) / 2 : center - height / 2
                color: modelData >= control.first.position && modelData <= control.second.position ? control.mdState.trackMarkColor : control.mdState.trackMarkInactiveColor
                visible: Math.abs(center - control.__firstCenter) > control.__firstGap + width / 2 && Math.abs(center - control.__secondCenter) > control.__secondGap + width / 2
            }
        }
    }
    component Track: MD.Rectangle {
        required property real start
        required property real end
        property bool outerStart: false
        property bool outerEnd: false
        x: control.horizontal ? start : control.leftPadding + (control.availableWidth - width) / 2
        y: control.horizontal ? control.topPadding + (control.availableHeight - height) / 2 : start
        width: control.horizontal ? Math.max(0, end - start) : control.__thickness
        height: control.horizontal ? control.__thickness : Math.max(0, end - start)
        visible: width > 0 && height > 0
        readonly property real startCorner: outerStart ? control.__thickness / 2 : MD.Token.slider.track_inside_corner
        readonly property real endCorner: outerEnd ? control.__thickness / 2 : MD.Token.slider.track_inside_corner
        corners: control.horizontal ? MD.Util.corners(startCorner, endCorner, startCorner, endCorner) : MD.Util.corners(startCorner, startCorner, endCorner, endCorner)
    }
}
