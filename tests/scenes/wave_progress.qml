import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

Rectangle {
    width: 700
    height: 400
    color: MD.MProp.color.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        MD.Text {
            text: "linear · determinate (default flat / wavy)"
            typescale: MD.Token.typescale.title_small
        }
        MD.LinearIndicator { Layout.fillWidth: true; value: 0.25 }
        MD.LinearIndicator { Layout.fillWidth: true; value: 0.5; wavy: true; __phase: 0.25 }
        MD.LinearIndicator { Layout.fillWidth: true; value: 0.85; wavy: true; __phase: 0.5 }

        MD.Text {
            text: "linear · indeterminate (default flat / wavy)"
            typescale: MD.Token.typescale.title_small
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: true
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: true
            wavy: true
            type: MD.LinearIndicator.Disjoint
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: true
            wavy: true
            type: MD.LinearIndicator.Contiguous
        }

        Item { Layout.preferredHeight: 8 }

        MD.Text {
            text: "circular · determinate (flat / wavy) and indeterminate"
            typescale: MD.Token.typescale.title_small
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 24

            MD.CircularIndicator {
                value: 0.65
                inactiveColor: MD.MProp.color.secondary_container
            }
            MD.CircularIndicator {
                value: 0.6; wavy: true; __phase: 0.3
                inactiveColor: MD.MProp.color.secondary_container
            }
            MD.CircularIndicator {
                indeterminate: true
            }
            MD.CircularIndicator {
                indeterminate: true
                wavy: true
            }
            MD.CircularIndicator {
                indeterminate: true
                wavy: true
                type: MD.CircularIndicator.Advance
            }
        }
    }
}
