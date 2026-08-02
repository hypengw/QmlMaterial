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

    Column {
        x: 456
        y: 16
        width: 248
        spacing: 12

        MD.RailItem {
            width: 96
            iconStyle: MD.Enum.IconOnly
            action: MD.Action {
                text: 'Settings'
                tooltip: 'Configure application'
                icon.name: MD.Token.icon.settings
                checkable: true
                checked: true
            }
        }

        MD.RailItem {
            expand: true
            iconStyle: MD.Enum.IconOnly
            action: MD.Action {
                text: 'Search'
                tooltip: 'Search library'
                icon.name: MD.Token.icon.search
                checkable: true
                checked: true
            }
        }
    }
}
