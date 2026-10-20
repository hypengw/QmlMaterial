import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 744
    height: 440
    color: MD.MProp.color.surface

    Grid {
        x: 24
        y: 24
        columns: 3
        spacing: 24

        Repeater {
            model: 6
            MD.Card {
                id: card
                required property int index
                width: 216
                height: 80
                type: [MD.Enum.CardElevated, MD.Enum.CardFilled, MD.Enum.CardOutlined][index % 3]
                enabled: index < 3
                contentItem: MD.Label {
                    text: ["Elevated", "Filled", "Outlined"][card.index % 3]
                    color: card.mdState.textColor
                    opacity: card.mdState.contentOpacity
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    MD.SheetActionBar {
        x: 24
        y: 240
        width: 696
        actions: [
            MD.Action {
                text: "Share"
                icon.name: "share"
            },
            MD.Action {
                text: "Archive"
                icon.name: "archive"
                checkable: true
                checked: true
            },
            MD.Action {
                text: "Delete"
                icon.name: "delete"
                enabled: false
            },
            MD.Action {
                text: "Add to collection"
                icon.name: "add"
            }
        ]
    }

    MD.SheetActionBar {
        x: 24
        y: 336
        width: 696
        enabled: false
        actions: [
            MD.Action {
                text: "Share"
                icon.name: "share"
            },
            MD.Action {
                text: "Archive"
                icon.name: "archive"
                checkable: true
                checked: true
            },
            MD.Action {
                text: "Add to collection"
                icon.name: "add"
            }
        ]
    }
}
