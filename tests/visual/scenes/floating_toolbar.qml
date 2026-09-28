import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 560
    height: 320
    color: MD.Token.color.surface

    component Toolbar: MD.FloatingToolbar {
        animationsEnabled: false
        mainContent: MD.IconButton {
            icon.name: "edit"
            mdState.type: MD.Enum.IBtFilledTonal
        }
        leadingContent: MD.IconButton {
            icon.name: "undo"
        }
        trailingContent: MD.IconButton {
            icon.name: "share"
        }
    }

    Toolbar {
        x: 24
        y: 24
    }
    Toolbar {
        x: 24
        y: 112
        expanded: false
    }
    Toolbar {
        x: 24
        y: 200
        expansionProgress: 0.5
    }
    Toolbar {
        x: 240
        y: 24
        orientation: Qt.Vertical
    }
    Toolbar {
        x: 360
        y: 24
        layoutDirection: Qt.RightToLeft
    }
    Toolbar {
        x: 360
        y: 112
        width: 100
        elevation: 2
    }
}
