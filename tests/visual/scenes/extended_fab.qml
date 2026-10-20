import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 440
    height: 300
    color: "#fff8f8"
    Column {
        x: 32
        y: 24
        spacing: 24
        MD.ExtendedFAB {
            MD.ActionCommand.action: MD.Action {
                text: "Create"
                icon.name: "add"
            }
            text: MD.ActionCommand.text
            icon.actionIcon: MD.ActionCommand.icon
            enabled: MD.ActionCommand.enabled
        }
        MD.ExtendedFAB {
            text: "Create"
            icon.name: "add"
            expanded: false
        }
        MD.ExtendedFAB {
            text: "Compose"
            width: 100
        }
    }
}
