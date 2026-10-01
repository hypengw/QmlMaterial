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
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "linear · determinate (default flat / wavy)"
            typescale: MD.Token.typescale.title_small
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: false
            value: 0.25
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: false
            value: 0.5
            wavy: true
            animationsEnabled: false
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: false
            value: 0.85
            wavy: true
            animationsEnabled: false
        }

        MD.Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "linear · indeterminate (default flat / wavy)"
            typescale: MD.Token.typescale.title_small
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: true
            animationsEnabled: false
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: true
            wavy: true
            type: MD.LinearIndicator.Disjoint
            animationsEnabled: false
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: true
            wavy: true
            type: MD.LinearIndicator.Contiguous
            animationsEnabled: false
        }

        Item {
            Layout.preferredHeight: 8
        }

        MD.Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "circular · determinate (flat / wavy) and indeterminate"
            typescale: MD.Token.typescale.title_small
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            MD.CircularIndicator {
                indeterminate: false
                value: 0.65
                inactiveColor: MD.MProp.color.secondary_container
            }
            MD.CircularIndicator {
                indeterminate: false
                value: 0.6
                wavy: true
                animationsEnabled: false
                inactiveColor: MD.MProp.color.secondary_container
            }
            MD.CircularIndicator {
                indeterminate: true
                animationsEnabled: false
            }
            MD.CircularIndicator {
                indeterminate: true
                wavy: true
                animationsEnabled: false
            }
            MD.CircularIndicator {
                indeterminate: true
                wavy: true
                type: MD.CircularIndicator.Advance
                animationsEnabled: false
            }
        }
        MD.LinearIndicator {
            Layout.fillWidth: true
            indeterminate: false
            animationsEnabled: false
            completionBehavior: MD.LinearIndicator.Keep
            value: 1
            wavy: true
        }
        RowLayout {
            Layout.fillWidth: true
            MD.LinearIndicator {
                Layout.fillWidth: true
                indeterminate: false
                animationsEnabled: false
                value: 0.005
                wavy: true
            }
            MD.CircularIndicator {
                indeterminate: false
                animationsEnabled: false
                completionBehavior: MD.CircularIndicator.Keep
                value: 1
                wavy: true
                inactiveColor: MD.MProp.color.secondary_container
            }
        }
    }
}
