import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 520
    height: 280
    color: MD.MProp.color.surface

    MD.BusyIconButton {
        x: 92
        y: 72
        icon.name: MD.Token.icon.upload
        busy: MD.Enum.Busy
    }

    MD.BusyIconButton {
        x: 172
        y: 72
        action: MD.Action {
            icon.name: MD.Token.icon.download
            busy: MD.Enum.Progress
            progress: 0.62
        }
    }

    MD.BusyButton {
        x: 252
        y: 76
        action: MD.Action {
            text: "Upload"
            icon.name: MD.Token.icon.upload
            busy: MD.Enum.Progress
            progress: 0.42
        }
    }

    MD.MenuItem {
        x: 132
        y: 160
        width: 256
        action: MD.Action {
            text: "Syncing"
            icon.name: MD.Token.icon.sync
            busy: MD.Enum.Progress
            progress: 0.78
        }
    }
}
