import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 800
    height: 600
    color: "#0a0a0a"

    // 6 cells exercising RippleSkia at different fixed progress values and
    // a variety of corner shapes to visually verify the SDF mask handles
    // pill, mixed, diagonal and asymmetric corners correctly.
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

                width: (root.width - 48 - 48) / 3
                height: (root.height - 48 - 24) / 2

                readonly property var progressList: [0.40, 0.40, 0.40, 0.40, 0.40, 0.40]

                // Per-cell corner specs — MD.Util.corners(tl, tr, bl, br).
                // Six distinct shapes stress every branch of the
                // sdf_rounded_rectangle mask used inside RippleSkia.
                readonly property var cornersList: [
                    MD.Util.corners(40, 40, 40, 40),                                // 0: uniform large radius
                    MD.Util.corners(60,  0,  0, 60),                                // 1: diagonal (TL + BR)
                    MD.Util.corners(height / 2, height / 2,
                                    height / 2, height / 2),                       // 2: pill / stadium
                    MD.Util.corners(72, 72,  0,  0),                                // 3: top-only rounded
                    MD.Util.corners( 0,  0, 72, 72),                                // 4: bottom-only rounded
                    MD.Util.corners(80,  8,  8, 80)                                 // 5: asymmetric (TL+BR big, TR+BL small)
                ]

                // Touch positions per cell — off-center so the ripple visibly
                // originates from somewhere other than the geometric center
                // and the touch→origin drift is obvious.
                readonly property var touchList: [
                    Qt.point(0.30, 0.45),
                    Qt.point(0.20, 0.25),
                    Qt.point(0.50, 0.50),
                    Qt.point(0.50, 0.30),
                    Qt.point(0.50, 0.70),
                    Qt.point(0.85, 0.20)
                ]

                // The container shares the ripple's corners so the outline is
                // visible against the dark canvas — that's what shows the SDF
                // mask is cutting through correctly.
                MD.Rectangle {
                    id: cellBg
                    anchors.fill: parent
                    color: "#4a4a4a"
                    corners: cell.cornersList[cell.index]
                }

                MD.RippleSkia {
                    anchors.fill: parent
                    color: "#5fe0a0"
                    stateOpacity: 0
                    corners: cellBg.corners

                    debugEnabled: true
                    debugProgress: cell.progressList[cell.index]
                    debugPhase: 5.0 + cell.index * 1.3
                    debugTouchX: cell.width  * cell.touchList[cell.index].x
                    debugTouchY: cell.height * cell.touchList[cell.index].y
                    debugOpacity: 0.85
                    debugBackOpacity: 0.12  // state-layer tint — what makes the press
                                            // feel shape-conforming in real Material UIs
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#ffffff"
                    font.pixelSize: 12
                    text: "p=" + cell.progressList[cell.index].toFixed(2)
                }
            }
        }
    }
}
