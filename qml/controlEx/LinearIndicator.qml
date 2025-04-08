import QtQuick
import QtQuick.Templates as T

import Qcm.Material as MD

T.BusyIndicator {
    id: control
    property real _progress: 0
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
        duration: 2000
        easing.type: Easing.InOutCubic
        function reset() {
            control._progress = 0;
        }
    }

    contentItem: Item {
        implicitHeight: 4
        implicitWidth: 100
        MD.LinearIndicatorShape {
            id: m_shape
            anchors.fill: parent

            Connections {
                target: control
                function on_ProgressChanged() {
                    const s = m_shape;
                    const p = control._progress;
                    const w = m_shape.width;
                    const primary = control.MD.MatProp.color.primary;
                    const container = control.MD.MatProp.color.secondary_container;

                    const t = p * 5;

                    if (t < 2) {
                        const x = t / 2;
                        s.c1 = container;
                        s.c2 = primary;
                        s.c3 = container;
                        s.x1 = 0.5 * x * w;
                        s.x2 = x * w;
                    } else if (t >= 2 && t < 3) {
                        const x = t - 2;
                        s.c1 = primary;
                        s.c2 = container;
                        s.c3 = primary;
                        s.x1 = (1 / 3) * x * w;
                        s.x2 = (0.5 + 0.5 * x) * w;
                    } else if (t >= 3 && t < 4) {
                        const x = (t - 3);
                        s.c1 = container;
                        s.c2 = primary;
                        s.c3 = container;
                        s.x1 = (1 / 3) * x * w;
                        s.x2 = ((1 / 3) + (2 / 3) * x) * w;
                    } else if (t >= 4 && t < 5) {
                        const x = (t - 4);
                        s.c1 = container;
                        s.c2 = primary;
                        s.c3 = container;
                        s.x1 = ((1 / 3) + (2 / 3) * x) * w;
                        s.x2 = w;
                    } else {}
                }
                function onRunningChanged() {
                    m_anim.running = true;
                    if (control.running) {
                        m_anim.paused = false;
                    } else {
                        m_anim.paused = true;
                    }
                }
            }
        }
    }

    background: Item {}
}
