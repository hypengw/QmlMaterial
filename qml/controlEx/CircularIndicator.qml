import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

T.BusyIndicator {
    id: control
    readonly property real arcDuration: 1333
    readonly property real fullArcDuration: 4 * 1333
    readonly property real fullDuration: 17 * 4 * 1333
    property real strokeWidth: 4
    property real _progress: 0

    // duration: (4 * control.arcDuration) / 360 * 306 -> 4532 -> 0.85 (360)
    // duration: (4 * control.arcDuration)             -> 5332 -> 1.0  (1080)
    // LCM -> 6041156, 17

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    clip: false

    NumberAnimation {
        id: m_anim
        loops: Animation.Infinite
        target: control
        property: '_progress'
        from: 0
        to: 1
        duration: control.fullDuration

        function reset() {
            control._progress = 0;
        }
    }

    contentItem: Item {
        implicitHeight: 32
        implicitWidth: 32
        MD.CircleIndicatorShape {
            id: m_shape
            anchors.fill: parent
            strokeWidth: control.strokeWidth

            Connections {
                target: control
                readonly property var _cal: (function () {
                        let lastStart = 0;
                        let lastEnd = 20;
                        return function () {
                            m_shape.rotation = control._progress * 17 * 360 * 3;
                            control.rotation = control._progress * 20 * 360;

                            const s = m_shape;
                            const p = (control._progress * 17 * 2) % 1;

                            if (p < 0.25) {
                                const t = p * 4;
                                s.endAngle = lastEnd + t * 300;
                            } else if (p >= 0.25 && p < 0.5) {
                                if (s.startAngle > 360 * 2) {
                                    s.startAngle = s.startAngle % 360;
                                    s.endAngle = s.startAngle + 320;
                                }
                                lastEnd = s.endAngle;
                                lastStart = s.startAngle;
                            } else if (p >= 0.5 && p < 0.75) {
                                const t = (p - 0.5) * 4;
                                s.startAngle = lastStart + t * 300;
                            } else {
                                if (s.startAngle > 360 * 2) {
                                    s.startAngle = s.startAngle % 360;
                                    s.endAngle = s.startAngle + 20;
                                }
                                lastEnd = s.endAngle;
                                lastStart = s.startAngle;
                            }
                        };
                    })()

                function on_ProgressChanged() {
                    _cal();
                }

                function onRunningChanged() {
                    m_anim.running = true;
                    if (control.running) {
                        m_anim.paused = false;
                    } else {
                        m_anim.paused = true;
                    }
                }

                Component.onCompleted: onRunningChanged()
            }
        }
    }

    background: Item {
        implicitHeight: 64
        implicitWidth: 64
    }
}
