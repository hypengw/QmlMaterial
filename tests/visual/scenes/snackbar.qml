import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 840
    height: 280
    color: MD.MProp.color.surface

    MD.Action {
        id: standardAction
        text: "Action"
    }

    MD.Action {
        id: longerAction
        text: "Longer action"
    }

    MD.SnakeBar {
        x: 20
        y: 20
        width: 344
        text: "Single-line snackbar with action"
        action: standardAction
    }

    MD.SnakeBar {
        x: 400
        y: 20
        width: 280
        text: "Two-line snackbar with action"
        action: standardAction
    }

    MD.SnakeBar {
        x: 20
        y: 100
        width: 344
        text: "Two-line snackbar\nwith longer action"
        action: longerAction
        actionOnNewLine: true
    }

    MD.SnakeBar {
        x: 400
        y: 100
        width: 420
        text: "Import completed with additional details. 128 records were processed, 124 were added, three duplicates were skipped, and one record needs manual review before it can be published."
        action: standardAction
    }
}
