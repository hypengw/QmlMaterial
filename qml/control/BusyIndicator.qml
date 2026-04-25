import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.BusyIndicator {
    id: control

    // Delay before the indicator appears (ms).
    // Avoids flicker for fast operations.
    property int showDelay: 0

    // Minimum time the indicator stays visible after show() was triggered (ms).
    // Capped at 1000 ms to match MDC-Android MAX_HIDE_DELAY.
    property int minHideDelay: 0

    // Logical diameter of the morphing shape in pixels.
    property int indicatorSize: 40

    property list<color> colors: [MD.MProp.color.primary]
    property color containerColor: "transparent"

    running: false
    visible: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    // ── Content ──────────────────────────────────────────────────────────────
    contentItem: Item {
        implicitWidth: 48
        implicitHeight: 48

        // Optional container background
        Rectangle {
            anchors.fill: parent
            color: control.containerColor
            radius: Math.min(width, height) / 2
            visible: control.containerColor !== "transparent"
        }

        MD.LoadingIndicatorUpdator {
            id: updator
            colors: control.colors
        }

        // Drive progress linearly; all easing is handled inside the C++ updator
        // via the analytic spring simulation — do NOT add extra easing here.
        // Wall-clock driven (FrameAnimation + Date.now) to stay refresh-rate
        // independent — Qt's animation driver ticks per-frame on threaded
        // render loops, so NumberAnimation runs ~Nx faster on Nx-Hz displays.
        FrameAnimation {
            id: morphAnim
            running: control.running && !showDelayTimer.running
            property real _startMs: 0
            onRunningChanged: { if (running) _startMs = Date.now(); }
            onTriggered: {
                const period = updator.shapeCount * updator.msPerShape;
                if (period > 0) {
                    const t = ((Date.now() - _startMs) % period) / period;
                    updator.progress = t * updator.shapeCount;
                }
            }
        }

        MD.BusyIndicatorShape {
            id: shape
            anchors.centerIn: parent
            scale: control.indicatorSize / 2

            color: updator.color
            rotation: updator.rotation
            points: updator.currentShape
        }
    }

    // ── Show / hide timing (mirrors MDC-Android LoadingIndicator) ────────────

    // Wall-clock time at which the indicator became visible.
    property real _showStartTime: 0

    Timer {
        id: showDelayTimer
        interval: Math.max(0, control.showDelay)
        onTriggered: {
            control.visible = true;
            control._showStartTime = Date.now();
        }
    }

    Timer {
        id: hideDelayTimer
        // interval is set dynamically to the remaining minHideDelay budget
        onTriggered: control.visible = false
    }

    onRunningChanged: {
        if (running) {
            hideDelayTimer.stop();

            if (showDelay > 0) {
                control.visible = false;
                showDelayTimer.restart();
            } else {
                showDelayTimer.stop();
                control.visible = true;
                control._showStartTime = Date.now();
            }
        } else {
            showDelayTimer.stop();

            if (minHideDelay > 0 && control.visible) {
                const elapsed = Date.now() - control._showStartTime;
                const cap = Math.min(control.minHideDelay, 1000); // MAX_HIDE_DELAY = 1000
                const remaining = cap - elapsed;
                if (remaining > 0) {
                    hideDelayTimer.interval = remaining;
                    hideDelayTimer.restart();
                    return;
                }
            }
            control.visible = false;
        }
    }
}
