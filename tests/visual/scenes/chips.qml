import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 720
    height: 220
    color: MD.MProp.color.surface

    Column {
        x: 24
        y: 24
        spacing: 32

        Repeater {
            model: 3
            delegate: Row {
                id: row
                required property int index
                spacing: 16
                enabled: index < 2

                MD.AssistChip {
                    text: "Assist"
                    icon.name: "event"
                    elevated: row.index == 1
                }
                MD.FilterChip {
                    text: "Filter"
                    selected: row.index > 0
                }
                MD.InputChip {
                    text: "Input"
                    selected: row.index > 0
                    trailingAction: MD.Action {
                        icon.name: "close"
                    }
                }
                MD.SuggestionChip {
                    text: "Suggestion"
                    elevated: row.index == 1
                }
                MD.EmbedChip {
                    text: "Embed"
                    icon.name: "description"
                    trailingIconName: "close"
                }
            }
        }
    }
}
