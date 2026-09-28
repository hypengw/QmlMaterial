import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 280
    height: 400
    color: MD.Token.color.surface
    MD.SearchBar {
        id: bar
        x: 12
        y: 40
        width: 256
        layoutDirection: Qt.RightToLeft
        searchText: 'بحث'
    }
    MD.SearchView {
        searchBar: bar
        presentation: MD.SearchView.FullScreen
        enter: null
        exit: null
        autoShowKeyboard: false
        Component.onCompleted: open()
        Column {
            width: parent.width
            Repeater {
                model: 10
                MD.MenuItem {
                    width: parent.width
                    text: 'نص طويل لنتيجة البحث في المكونات'
                    icon.name: 'search'
                }
            }
        }
    }
}
