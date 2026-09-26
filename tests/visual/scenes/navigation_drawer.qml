import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 720
    height: 540
    color: MD.Token.color.surface
    Timer {
        interval: 1200
        running: true
        onTriggered: rail.close()
    }

    MD.NavigationRail {
        id: rail
        x: 16
        y: 12
        height: parent.height - 24
        forceModal: true
        autoExpand: false
        arrangement: MD.Enum.RailTop
        model: [
            {
                name: "Home",
                icon: MD.Token.icon.home
            },
            {
                name: "Search",
                icon: MD.Token.icon.search
            },
            {
                section: "Library"
            },
            {
                name: "Settings",
                icon: MD.Token.icon.settings
            }
        ]
        fabAction: MD.Action {
            text: "Create"
            icon.name: MD.Token.icon.add
        }
        Component.onCompleted: Qt.callLater(open)
    }
}
