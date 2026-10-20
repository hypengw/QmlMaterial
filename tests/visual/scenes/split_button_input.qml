pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 640
    height: 480
    color: MD.Token.color.surface

    MD.SplitButton {
        x: 24
        y: 24
        text: "Save"
        icon.name: MD.Token.icon.save
        mdState.type: MD.Enum.BtFilled
    }
    MD.SplitButton {
        x: 280
        y: 24
        text: "Share"
        mdState.type: MD.Enum.BtFilledTonal
        mdState.size: MD.Enum.M
    }
    MD.SplitButton {
        x: 24
        y: 120
        text: "Edit"
        mdState.isRound: false
    }
    MD.SplitButton {
        x: 280
        y: 120
        text: "Unavailable"
        enabled: false
    }
    MD.SplitButton {
        id: expanded
        x: 24
        y: 224
        text: "Options"
        menu: MD.Menu {
            enter: null
            exit: null
            MD.MenuItem {
                text: "Save a copy"
            }
            MD.MenuItem {
                text: "Move to folder"
            }
            MD.MenuItem {
                text: "Export"
            }
        }
    }
    function showMenu() {
        if (Window.window && !expanded.menu.visible)
            expanded.menu.openFor(expanded);
    }
    Connections {
        target: root.Window.window
        function onWidthChanged() {
            root.showMenu();
        }
        function onHeightChanged() {
            root.showMenu();
        }
    }
    Component.onCompleted: Qt.callLater(showMenu)
}
