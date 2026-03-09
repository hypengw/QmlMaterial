pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Slider {
    id: control

    property MD.StateSlider mdState: MD.StateSlider {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitHandleHeight + topPadding + bottomPadding)

    clip: false

    // Cursor handling
    PointHandler {
        id: mouseHandler
        cursorShape: control.pressed ? Qt.ClosedHandCursor : (control.hovered ? Qt.PointingHandCursor : Qt.ArrowCursor)
    }

    readonly property real __steps: Math.abs(to - from) / stepSize
    readonly property bool __isDiscrete: stepSize >= Number.EPSILON && snapMode === Slider.SnapAlways && Math.abs(Math.round(__steps) - __steps) < Number.EPSILON
    readonly property real handleCenter: {
        const w = control.horizontal ? control.availableWidth : control.availableHeight;
        const hw = control.mdState.handleWidth;
        const start = control.horizontal ? control.leftPadding : control.topPadding;
        return control.visualPosition * (w - hw) + hw / 2 + start;
    }

    handle: MD.SliderHandle {
        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
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
            x: control.horizontal ? control.handleCenter + bgItem.gapSize : (bgItem.width - 16) / 2
            y: control.horizontal ? (bgItem.height - 16) / 2 : 0
            width: control.horizontal ? Math.max(0, bgItem.width - x) : 16
            height: control.horizontal ? 16 : Math.max(0, control.handleCenter - bgItem.gapSize - y)
            corners: control.horizontal ? MD.Util.corners(2, 8, 2, 8) : MD.Util.corners(8, 8, 2, 2)
            color: control.mdState.trackInactiveColor
            visible: width > 0 && height > 0
        }

        // Active Track (Left for horizontal, Bottom for vertical)
        MD.Rectangle {
            id: activeTrack
            x: control.horizontal ? 0 : (bgItem.width - 16) / 2
            y: control.horizontal ? (bgItem.height - 16) / 2 : control.handleCenter + bgItem.gapSize
            width: control.horizontal ? Math.max(0, control.handleCenter - bgItem.gapSize - x) : 16
            height: control.horizontal ? 16 : Math.max(0, bgItem.height - y)
            corners: control.horizontal ? MD.Util.corners(8, 2, 8, 2) : MD.Util.corners(2, 2, 8, 8)
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
                const diff_to_end = (control.horizontal ? parent.width - control.handleCenter - control.rightPadding : control.handleCenter) - control.mdState.handleWidth / 2 - 2;
                return Math.min(Math.max(diff_to_end / 2, 0), 1);
            }
            x: control.horizontal ? (parent.width - (control.mdState.handleWidth + 4) / 2) - control.rightPadding : (parent.width - 4) / 2
            y: control.horizontal ? (parent.height - 4) / 2 : (control.mdState.handleWidth - 4) / 2
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
                        const w = control.availableWidth;
                        const hw = control.mdState.handleWidth;
                        const pos = currentPosition * (w - hw) + hw / 2 + control.leftPadding;
                        return pos - width / 2;
                    } else
                        return (bgItem.width - width) / 2;
                }
                y: {
                    if (!control.horizontal) {
                        const w = control.availableHeight;
                        const hw = control.mdState.handleWidth;
                        const pos = currentPosition * (w - hw) + hw / 2 + control.topPadding;
                        return pos - height / 2;
                    } else
                        return (bgItem.height - height) / 2;
                }
                color: active ? control.mdState.trackMarkColor : control.mdState.trackMarkInactiveColor

                readonly property real dotPos: control.horizontal ? x : y
                readonly property real gapStart: control.handleCenter - bgItem.gapSize
                readonly property real gapEnd: control.handleCenter + bgItem.gapSize
                visible: dotPos < gapStart || dotPos > (gapEnd - 4)

                required property int index
                readonly property real currentPosition: index / (control.__steps)
                readonly property bool active: (control.horizontal && control.visualPosition > currentPosition) || (!control.horizontal && control.visualPosition <= currentPosition)
            }
        }
    }
}
