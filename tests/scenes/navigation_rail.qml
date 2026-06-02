import QtQuick
import Qcm.Material as MD

Item {
    id: root
    width: 720
    height: 480

    readonly property var navModel: [
        {
            name: 'home',
            icon: MD.Token.icon.home
        },
        {
            name: 'search',
            icon: MD.Token.icon.search
        },
        {
            section: 'Library'
        },
        {
            name: 'settings',
            icon: MD.Token.icon.settings
        }
    ]

    Rectangle {
        anchors.fill: parent
        color: MD.Token.color.surface
    }

    // collapsed wide rail (96dp, vertical items)
    MD.NavigationRail {
        id: m_collapsed
        x: 0
        y: 0
        height: parent.height
        model: root.navModel
        currentIndex: 1
        arrangement: MD.Enum.RailTop
        fabAction: MD.Action {
            icon.name: MD.Token.icon.add
            text: 'Add'
        }
    }

    // expanded items (horizontal layout) shown directly
    Column {
        x: 160
        y: 16
        width: 260
        spacing: 4

        Repeater {
            model: root.navModel
            MD.RailItem {
                required property var modelData
                required property int index
                visible: modelData.section === undefined
                width: 260
                expand: true
                icon.name: modelData.icon ?? ''
                text: modelData.name ?? ''
                checked: index === 1
            }
        }
    }
}
