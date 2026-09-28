import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 720
    height: 600
    color: MD.Token.color.surface
    Column {
        x: 16
        y: 16
        spacing: 24
        Repeater {
            model: [0, -44, -88]
            MD.AppBar {
                required property real modelData
                width: 688
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
            width: 280
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
    }
}
