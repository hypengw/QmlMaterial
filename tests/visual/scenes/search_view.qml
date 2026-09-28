import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 600
    height: 480
    color: MD.Token.color.surface
    MD.SearchBar {
        id: bar
        x: 80
        y: 48
        width: 400
        searchText: 'Material'
    }
    MD.SearchView {
        id: view
        searchBar: bar
        enter: null
        exit: null
        autoShowKeyboard: false
        Component.onCompleted: open()
        MD.VerticalFlickable {
            anchors.fill: parent
            Column {
                width: parent.width
                Repeater {
                    model: 12
                    MD.MenuItem {
                        required property int index
                        width: parent.width
                        text: 'Material result ' + (index + 1)
                        icon.name: 'search'
                    }
                }
            }
        }
    }
}
