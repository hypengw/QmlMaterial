pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 800
    height: 600
    color: MD.Token.color.surface

    MD.Label {
        x: 24
        y: 24
        text: "Anchored menus"
    }
    MD.Button {
        id: sortButton
        x: root.width - width - 24
        y: 24
        text: "Added time"
    }
    MD.Menu {
        id: sortMenu
        anchorItem: sortButton
        anchorAlignment: MD.PopupBase.End
        enter: null
        exit: null
        MD.MenuItem {
            text: "Title"
        }
        MD.MenuItem {
            text: "Added time"
            icon.name: MD.Token.icon.arrow_downward
        }
        MD.MenuItem {
            text: "Sort title"
        }
        MD.MenuItem {
            text: "Publish time"
        }
        MD.MenuItem {
            text: "Year"
        }
        MD.MenuItem {
            text: "Disc count"
        }
        MD.MenuItem {
            text: "Track count"
        }
    }
    MD.IconButton {
        id: bottomButton
        x: 24
        y: root.height - height - 24
        icon.name: MD.Token.icon.more_vert
    }
    MD.Menu {
        id: bottomMenu
        anchorItem: bottomButton
        preferredPlacement: MD.PopupBase.Above
        enter: null
        exit: null
        MD.MenuItem {
            text: "Play next"
        }
        MD.MenuItem {
            text: "Add to playlist"
        }
        MD.MenuItem {
            text: "Album"
        }
    }
    Connections {
        target: root.Window.window
        function onWidthChanged() {
            showMenus();
        }
        function onHeightChanged() {
            showMenus();
        }
    }
    function showMenus() {
        if (Window.window) {
            sortMenu.open();
            bottomMenu.open();
        }
    }
    Component.onCompleted: Qt.callLater(showMenus)
}
