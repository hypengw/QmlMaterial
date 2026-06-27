pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Slider {
    id: control

    property MD.StateSlider mdState: MD.StateSlider {
        item: control
    }
    property Component tailing: null
    property real tailingSpacing: 8

    readonly property real __tailingWidth: tailingLoader.active ? tailingLoader.width : 0
    readonly property real __tailingReserve: control.horizontal && __tailingWidth > 0 ? __tailingWidth + tailingSpacing : 0

    rightPadding: __tailingReserve

    property int labelBehavior: MD.Enum.SliderLabelFloating
    property int tickVisibilityMode: MD.Enum.SliderTickAutoLimit
    property int sliderSize: MD.Enum.SliderSizeXSmall

    property string insetIcon: ''
    property string insetIconAtMin: ''
    property real insetIconSize: MD.Token.slider.inset_icon_size
    property color insetIconActiveColor: control.mdState.textColor
    property color insetIconInactiveColor: control.mdState.trackMarkInactiveColor

    implicitWidth: {
        const contentWidth = implicitBackgroundWidth + leftInset + rightInset;
        const handleContentWidth = implicitHandleWidth + leftPadding + rightPadding;
        return Math.max(contentWidth, handleContentWidth);
    }
    implicitHeight: {
        const contentHeight = implicitBackgroundHeight + topInset + bottomInset;
        const handleContentHeight = implicitHandleHeight + topPadding + bottomPadding;
        return Math.max(contentHeight, handleContentHeight);
    }

    // Cursor handling
    PointHandler {
        cursorShape: {
            if (control.pressed)
                return Qt.ClosedHandCursor;
            if (control.hovered)
                return Qt.PointingHandCursor;
            return Qt.ArrowCursor;
        }
    }

    Loader {
        id: tailingLoader
        active: control.horizontal && control.tailing !== null
        visible: active
        sourceComponent: control.tailing
        x: control.width - width
        y: (control.height - height) / 2
        z: 1
    }

    readonly property real __steps: Math.abs(to - from) / stepSize
    readonly property bool __isDiscrete: stepSize >= Number.EPSILON && snapMode === Slider.SnapAlways && Math.abs(Math.round(__steps) - __steps) < Number.EPSILON

    // Cap visible tick dots so wide ranges (e.g. 1..240 step 1) don't
    // collapse into a solid line. Snap behavior stays driven by
    // `stepSize`; this only affects how many decorative dots are drawn.
    property int maxVisibleStops: 20

    readonly property bool __showStopIndicators: control.tickVisibilityMode !== MD.Enum.SliderTickNone
    readonly property bool __showContinuousEndCaps: control.__showStopIndicators
            && !control.__isDiscrete
            && control.tickVisibilityMode === MD.Enum.SliderTickAll

    readonly property int __dotCount: {
        if (!control.__isDiscrete || !control.__showStopIndicators)
            return 0;
        const total = Math.floor(control.__steps);
        if (control.tickVisibilityMode === MD.Enum.SliderTickAll)
            return total + 1;
        return Math.min(total, control.maxVisibleStops) + 1;
    }
    readonly property real __dotStep: control.__dotCount > 1 ? 1.0 / (control.__dotCount - 1) : 0

    readonly property real __trackGapSize: MD.Token.slider.thumb_track_gap + control.mdState.handleLineWidth / 2

    readonly property real __activeTrackThickness: {
        switch (control.sliderSize) {
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
    readonly property real __inactiveTrackThickness:
        control.sliderSize >= MD.Enum.SliderSizeMedium
            ? control.__activeTrackThickness
            : MD.Token.slider.inactive_track_height
    readonly property real __stopSize: MD.Token.slider.stop_indicator_size
    readonly property real __insideCorner: MD.Token.slider.track_inside_corner
    readonly property real __outerCorner: control.__activeTrackThickness / 2

    readonly property bool __hasInsetIcon: control.insetIcon.length > 0
            && control.sliderSize >= MD.Enum.SliderSizeMedium

    readonly property string __resolvedInsetIcon:
        control.insetIconAtMin.length > 0 && control.value <= control.from + Number.EPSILON
            ? control.insetIconAtMin
            : control.insetIcon

    readonly property real handleCenter: {
        const trackLength = control.horizontal ? control.availableWidth : control.availableHeight;
        const handleSize = control.mdState.handleWidth;
        const paddingOffset = control.horizontal ? control.leftPadding : control.topPadding;
        return control.visualPosition * (trackLength - handleSize) + handleSize / 2 + paddingOffset;
    }

    readonly property real __horizontalActiveTrackLength:
        control.handleCenter - control.leftPadding - control.__trackGapSize

    readonly property real __inactiveTopLength:
        control.handleCenter - control.__trackGapSize - control.topPadding

    readonly property real __insetIconDivider: control.handleCenter + control.__trackGapSize

    readonly property real __insetIconPad: MD.Token.slider.inset_icon_padding

    readonly property bool __insetIconOnActiveTrack:
        control.horizontal
            && control.__horizontalActiveTrackLength
                    >= control.insetIconSize + 2 * control.__insetIconPad

    readonly property bool __verticalInsetIconFitsAtTop:
        control.__inactiveTopLength >= control.insetIconSize + 2 * control.__insetIconPad

    readonly property real __insetIconX: {
        if (control.horizontal) {
            if (control.__insetIconOnActiveTrack) {
                return control.leftPadding + control.__insetIconPad;
            }
            return control.handleCenter + control.__trackGapSize + control.__insetIconPad;
        }
        return (control.implicitBackgroundWidth - control.insetIconSize) / 2;
    }

    readonly property real __insetIconY: {
        if (control.horizontal) {
            return (control.implicitBackgroundHeight - control.insetIconSize) / 2;
        }
        if (control.__verticalInsetIconFitsAtTop) {
            return control.__insetIconPad;
        }
        return control.handleCenter + control.__trackGapSize + control.__insetIconPad;
    }

    readonly property color __insetIconColor: {
        if (control.horizontal) {
            return control.__insetIconOnActiveTrack
                    ? control.insetIconActiveColor
                    : control.insetIconInactiveColor;
        }
        if (!control.__verticalInsetIconFitsAtTop) {
            return control.insetIconActiveColor;
        }
        const iconCenterY = control.__insetIconPad + control.insetIconSize / 2;
        return iconCenterY >= control.__insetIconDivider
                ? control.insetIconActiveColor
                : control.insetIconInactiveColor;
    }

    handle: MD.SliderHandle {
        // Above background so the separator line paints over active/inactive tracks.
        z: 2
        x: {
            const availableSpace = control.availableWidth - width;
            const offset = control.horizontal ? control.visualPosition * availableSpace : availableSpace / 2;
            return control.leftPadding + offset;
        }
        y: {
            const availableSpace = control.availableHeight - height;
            const offset = control.horizontal ? availableSpace / 2 : control.visualPosition * availableSpace;
            return control.topPadding + offset;
        }
        value: control.value
        labelBehavior: control.labelBehavior
        handleHasFocus: control.visualFocus
        handlePressed: control.pressed
        handleHovered: control.hovered
        horizontal: control.horizontal
        handleWidth: control.mdState.handleWidth
        handleHeight: control.mdState.handleHeight
        handleLineWidth: control.mdState.handleLineWidth
    }

    background: Item {
        id: bgItem
        opacity: control.mdState.backgroundOpacity
        implicitWidth: control.horizontal
            ? 200
            : Math.max(control.mdState.handleHeight, control.__activeTrackThickness)
        implicitHeight: control.horizontal
            ? Math.max(control.mdState.handleHeight, control.__activeTrackThickness)
            : 200

        readonly property real gapSize: control.__trackGapSize

        // Inactive Track (right for horizontal, top for vertical)
        MD.Rectangle {
            x: {
                if (control.horizontal) {
                    return control.handleCenter + bgItem.gapSize;
                }
                return (bgItem.width - control.__inactiveTrackThickness) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (bgItem.height - control.__inactiveTrackThickness) / 2;
                }
                return 0;
            }
            width: {
                if (control.horizontal) {
                    const trackEnd = bgItem.width - control.rightPadding;
                    return Math.max(0, trackEnd - x);
                }
                return control.__inactiveTrackThickness;
            }
            height: {
                if (control.horizontal) {
                    return control.__inactiveTrackThickness;
                }
                return Math.max(0, control.handleCenter - bgItem.gapSize - y);
            }
            corners: {
                const r = control.__inactiveTrackThickness / 2;
                return MD.Util.corners(r, r, r, r);
            }
            color: control.mdState.trackInactiveColor
            visible: width > 0 && height > 0
        }

        // Active Track (left for horizontal, bottom for vertical)
        MD.Rectangle {
            x: {
                if (control.horizontal) {
                    return 0;
                }
                return (bgItem.width - control.__activeTrackThickness) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (bgItem.height - control.__activeTrackThickness) / 2;
                }
                return control.handleCenter + bgItem.gapSize;
            }
            width: {
                if (control.horizontal) {
                    return Math.max(0, control.handleCenter - bgItem.gapSize - x);
                }
                return control.__activeTrackThickness;
            }
            height: {
                if (control.horizontal) {
                    return control.__activeTrackThickness;
                }
                return Math.max(0, bgItem.height - y);
            }
            corners: {
                if (control.horizontal) {
                    return MD.Util.corners(control.__outerCorner, control.__insideCorner,
                                           control.__outerCorner, control.__insideCorner);
                }
                return MD.Util.corners(control.__insideCorner, control.__insideCorner,
                                       control.__outerCorner, control.__outerCorner);
            }
            color: control.mdState.trackColor
            visible: width > 0 && height > 0
        }

        MD.Icon {
            z: 1
            visible: control.__hasInsetIcon
            name: control.__resolvedInsetIcon
            size: control.insetIconSize
            color: control.__insetIconColor
            x: control.__insetIconX
            y: control.__insetIconY
        }

        // Stop indicator at track start (continuous + tickVisibilityMode All only)
        MD.Rectangle {
            width: control.__stopSize
            height: control.__stopSize
            radius: control.__stopSize / 2
            scale: {
                const centerPos = control.handleCenter;
                const handleHalfWidth = control.mdState.handleWidth / 2;
                const distToStart = control.horizontal
                        ? centerPos - control.leftPadding
                        : (bgItem.height - centerPos - control.bottomPadding);
                const clearDistance = distToStart - handleHalfWidth - 2;
                return Math.min(Math.max(clearDistance / 2, 0), 1);
            }
            x: {
                if (control.horizontal) {
                    const stopOffset = (control.mdState.handleWidth + control.__stopSize) / 2;
                    return control.leftPadding + stopOffset - width / 2;
                }
                return (bgItem.width - control.__stopSize) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (bgItem.height - control.__stopSize) / 2;
                }
                const trackEnd = bgItem.height - control.bottomPadding;
                const stopOffset = (control.mdState.handleWidth + control.__stopSize) / 2;
                return trackEnd - stopOffset - height / 2;
            }
            color: control.mdState.trackMarkInactiveColor
            visible: control.__showContinuousEndCaps
        }

        // Stop indicator at track end (continuous + tickVisibilityMode All only)
        MD.Rectangle {
            width: control.__stopSize
            height: control.__stopSize
            radius: control.__stopSize / 2
            scale: {
                const centerPos = control.handleCenter;
                const handleHalfWidth = control.mdState.handleWidth / 2;
                const distToEnd = control.horizontal
                        ? (bgItem.width - centerPos - control.rightPadding)
                        : centerPos;
                const clearDistance = distToEnd - handleHalfWidth - 2;
                return Math.min(Math.max(clearDistance / 2, 0), 1);
            }
            x: {
                if (control.horizontal) {
                    const trackEnd = bgItem.width - control.rightPadding;
                    const stopOffset = (control.mdState.handleWidth + control.__stopSize) / 2;
                    return trackEnd - stopOffset - width / 2;
                }
                return (bgItem.width - control.__stopSize) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (bgItem.height - control.__stopSize) / 2;
                }
                const stopOffset = (control.mdState.handleWidth - control.__stopSize) / 2;
                return stopOffset;
            }
            color: control.mdState.trackMarkInactiveColor
            visible: control.__showContinuousEndCaps
        }

        // Discrete stop indicators
        Repeater {
            model: control.__dotCount
            delegate: MD.Rectangle {
                width: control.__stopSize
                height: control.__stopSize
                radius: control.__stopSize / 2
                x: {
                    if (control.horizontal) {
                        const trackWidth = control.availableWidth;
                        const handleWidth = control.mdState.handleWidth;
                        const dotCenter = currentPosition * (trackWidth - handleWidth) + handleWidth / 2 + control.leftPadding;
                        return dotCenter - width / 2;
                    }
                    return (bgItem.width - width) / 2;
                }
                y: {
                    if (!control.horizontal) {
                        const trackHeight = control.availableHeight;
                        const handleWidth = control.mdState.handleWidth;
                        const dotCenter = currentPosition * (trackHeight - handleWidth) + handleWidth / 2 + control.topPadding;
                        return dotCenter - height / 2;
                    }
                    return (bgItem.height - height) / 2;
                }
                color: active ? control.mdState.trackMarkColor : control.mdState.trackMarkInactiveColor

                readonly property real dotPos: control.horizontal ? x : y
                readonly property real gapStart: control.handleCenter - bgItem.gapSize
                readonly property real gapEnd: control.handleCenter + bgItem.gapSize
                visible: dotPos < gapStart || dotPos > (gapEnd - control.__stopSize)

                required property int index
                readonly property real currentPosition: index * control.__dotStep
                readonly property bool active: {
                    if (control.horizontal) {
                        return control.visualPosition > currentPosition;
                    }
                    return control.visualPosition <= currentPosition;
                }
            }
        }
    }
}
