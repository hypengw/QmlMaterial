import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 600
    height: 400
    color: "#E0E0E0"

    // Top row: adjacent rounded corners (TL + BL) at 5 elevation levels.
    // Bottom row: diagonal rounded corners (TL + BR) — exercises the case
    // where opposite corners are rounded and the other diagonal is square.
    Repeater {
        model: 10

        Item {
            required property int index

            width: 50
            height: 50
            x: 30 + (index % 5) * 110
            y: index < 5 ? 80 : 220

            MD.Rectangle {
                id: box
                anchors.fill: parent
                color: "transparent"
                // index 0..4 → adjacent (TL+BL), index 5..9 → diagonal (TL+BR)
                corners: (index < 5)
                    ? MD.Util.corners(20, 0, 20, 0)
                    : MD.Util.corners(20, 0, 0, 20)

                MD.Elevation {
                    anchors.fill: parent
                    elevation: MD.Token.elevation["level" + ((index % 5) + 1)]
                    corners: box.corners
                    z: -1
                }
            }
        }
    }
}
