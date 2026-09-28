import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 480
    height: 400
    color: MD.Token.color.surface
    MD.SearchBar {
        id: bar
        x: 64
        y: 64
        width: 320
        searchText: 'Material'
    }
    MD.SearchView {
        searchBar: bar
        presentation: MD.SearchView.FullScreen
        deferredCompletion: true
        autoShowKeyboard: false
        onAboutToShow: expansion = 0.5
        Component.onCompleted: open()
        Column {
            width: parent.width
            Repeater {
                model: 8
                MD.MenuItem {
                    width: parent.width
                    text: 'Material components'
                    icon.name: 'search'
                }
            }
        }
    }
}
