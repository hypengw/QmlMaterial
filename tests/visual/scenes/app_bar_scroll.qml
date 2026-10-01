import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 720
    height: 920
    color: MD.Token.color.surface
    Column {
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 24
        Repeater {
            model: [0, -22, -44, -66, -88]
            MD.AppBar {
                required property real modelData
                width: parent.width
                type: MD.Enum.AppBarLarge
                title: 'A library of places and memories'
                animationsEnabled: false
                scrollBehavior: MD.AppBarScroll {
                    collapseDistance: 88
                    heightOffset: modelData
                }
                leadingAction: MD.Action {
                    icon.name: 'arrow_back'
                }
                actions: [
                    MD.Action {
                        icon.name: 'search'
                    },
                    MD.Action {
                        icon.name: 'more_vert'
                    }
                ]
            }
        }
        MD.AppBar {
            width: Math.min(280, parent.width)
            layoutDirection: Qt.RightToLeft
            type: MD.Enum.AppBarMedium
            title: 'A long title with actions'
            animationsEnabled: false
            scrollBehavior: MD.AppBarScroll {
                collapseDistance: 48
                heightOffset: -48
            }
            leadingAction: MD.Action {
                icon.name: 'arrow_back'
            }
            actions: [
                MD.Action {
                    icon.name: 'search'
                }
            ]
        }
        MD.AppBar {
            width: parent.width
            type: MD.Enum.AppBarSmall
            title: 'Pinned over scrolled content'
            animationsEnabled: false
            scrollBehavior: MD.AppBarScroll {
                mode: MD.AppBarScroll.Pinned
                collapseDistance: 64
                contentAtStart: false
            }
        }
    }
}
