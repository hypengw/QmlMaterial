pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Slider {
    id: control

    property MD.StateSlider mdState: MD.StateSlider {
        item: control
    }

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

    clip: false

    // Cursor handling
    PointHandler {
        id: mouseHandler
        cursorShape: {
            if (control.pressed)
                return Qt.ClosedHandCursor;
            if (control.hovered)
                return Qt.PointingHandCursor;
            return Qt.ArrowCursor;
        }
    }

    readonly property real __steps: Math.abs(to - from) / stepSize
    readonly property bool __isDiscrete: stepSize >= Number.EPSILON && snapMode === Slider.SnapAlways && Math.abs(Math.round(__steps) - __steps) < Number.EPSILON
    readonly property real handleCenter: {
        const trackLength = control.horizontal ? control.availableWidth : control.availableHeight;
        const handleSize = control.mdState.handleWidth;
        const paddingOffset = control.horizontal ? control.leftPadding : control.topPadding;
        return control.visualPosition * (trackLength - handleSize) + handleSize / 2 + paddingOffset;
    }

    handle: MD.SliderHandle {
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
        implicitWidth: control.horizontal ? 200 : 44
        implicitHeight: control.horizontal ? 44 : 200

        readonly property bool isHandleSmall: control.pressed || control.visualFocus
        readonly property real gapSize: 6 + control.mdState.handleLineWidth / 2

        // Inactive Track (Right for horizontal, Top for vertical)
        MD.Rectangle {
            id: inactiveTrack
            x: {
                if (control.horizontal) {
                    return control.handleCenter + bgItem.gapSize;
                }
                return (bgItem.width - 16) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (bgItem.height - 16) / 2;
                }
                return 0;
            }
            width: {
                if (control.horizontal) {
                    return Math.max(0, bgItem.width - x);
                }
                return 16;
            }
            height: {
                if (control.horizontal) {
                    return 16;
                }
                return Math.max(0, control.handleCenter - bgItem.gapSize - y);
            }
            corners: {
                if (control.horizontal) {
                    return MD.Util.corners(2, 8, 2, 8);
                }
                return MD.Util.corners(8, 8, 2, 2);
            }
            color: control.mdState.trackInactiveColor
            visible: width > 0 && height > 0
        }

        // Active Track (Left for horizontal, Bottom for vertical)
        MD.Rectangle {
            id: activeTrack
            x: {
                if (control.horizontal) {
                    return 0;
                }
                return (bgItem.width - 16) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (bgItem.height - 16) / 2;
                }
                return control.handleCenter + bgItem.gapSize;
            }
            width: {
                if (control.horizontal) {
                    return Math.max(0, control.handleCenter - bgItem.gapSize - x);
                }
                return 16;
            }
            height: {
                if (control.horizontal) {
                    return 16;
                }
                return Math.max(0, bgItem.height - y);
            }
            corners: {
                if (control.horizontal) {
                    return MD.Util.corners(8, 2, 8, 2);
                }
                return MD.Util.corners(2, 2, 8, 8);
            }
            color: control.mdState.trackColor
            visible: width > 0 && height > 0
        }
        // Stop Indicators
        MD.Rectangle {
            id: stopEnd
            width: 4
            height: 4
            radius: 2
            scale: {
                const centerPos = control.handleCenter;
                const handleHalfWidth = control.mdState.handleWidth / 2;
                const distToEnd = control.horizontal ? (parent.width - centerPos - control.rightPadding) : centerPos;
                const clearDistance = distToEnd - handleHalfWidth - 2;
                return Math.min(Math.max(clearDistance / 2, 0), 1);
            }
            x: {
                if (control.horizontal) {
                    const trackEnd = parent.width - control.rightPadding;
                    const stopOffset = (control.mdState.handleWidth + 4) / 2;
                    return trackEnd - stopOffset;
                }
                return (parent.width - 4) / 2;
            }
            y: {
                if (control.horizontal) {
                    return (parent.height - 4) / 2;
                }
                const stopOffset = (control.mdState.handleWidth - 4) / 2;
                return stopOffset;
            }
            color: control.mdState.trackMarkInactiveColor
            visible: !control.__isDiscrete
        }

        // Discrete dots
        Repeater {
            model: control.__isDiscrete ? Math.floor(control.__steps) + 1 : 0
            delegate: MD.Rectangle {
                width: 4
                height: 4
                radius: 2
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
                visible: dotPos < gapStart || dotPos > (gapEnd - 4)

                required property int index
                readonly property real currentPosition: index / (control.__steps)
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
