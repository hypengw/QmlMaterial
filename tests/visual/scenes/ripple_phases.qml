import QtQuick
import Qcm.Material as MD

// Single shape, six progress samples → visualises the full ripple lifecycle.
// At low progress the wave is a small circle near the touch point; by
// mid-progress (≈ 0.4) it's at peak alpha; past 0.5 the circle exceeds the
// shape and the SDF mask + state-layer tint fill the cell uniformly while
// the wave fades — that's the "shape-conforming" perception.
Rectangle {
    id: root
    width: 800
    height: 600
    color: "#0a0a0a"

    Grid {
        anchors.fill: parent
        anchors.margins: 24
        columns: 3
        rowSpacing: 24
        columnSpacing: 24

        Repeater {
            model: 6

            delegate: Item {
                id: cell
                required property int index
                readonly property real progressVal: [0.10, 0.25, 0.40, 0.55, 0.70, 0.85][index]

                width: (root.width - 48 - 48) / 3
                height: (root.height - 48 - 24) / 2

                MD.Rectangle {
                    id: cellBg
                    anchors.fill: parent
                    color: "#4a4a4a"
                    corners: MD.Util.corners(40, 40, 40, 40)
                }

                MD.RippleSkia {
                    anchors.fill: parent
                    color: "#5fe0a0"
                    stateOpacity: 0
                    corners: cellBg.corners

                    debugEnabled: true
                    debugProgress: cell.progressVal
                    debugPhase: 4.0 + cell.index * 0.7
                    debugTouchX: cell.width  * 0.30
                    debugTouchY: cell.height * 0.45
                    debugOpacity: 0.85
                    debugBackOpacity: 0.12  // state-layer tint, like a real press
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#ffffff"
                    font.pixelSize: 12
                    text: "p=" + cell.progressVal.toFixed(2)
                }
            }
        }
    }
}
