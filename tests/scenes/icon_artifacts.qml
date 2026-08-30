import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

// Visual regression repro for MD.Icon on vertical-stroke glyphs (copy, delete, videocam).
// Top row: IconButton → MD.Icon (lineHeight = pixelSize, default renderType).
// Middle row: FAB-style Text reference (lineHeight = pixelSize).
// Bottom: raw MD.Icon at typical iconSize values (18 / 20 / 24).
Rectangle {
    id: root
    width: 560
    height: 420
    color: MD.Token.color.surface

    readonly property var artifactIcons: [
        { label: "content_copy", name: MD.Token.icon.content_copy },
        { label: "delete", name: MD.Token.icon.delete },
        { label: "videocam", name: MD.Token.icon.videocam }
    ]
    readonly property var iconSizes: [18, 20, 24]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        MD.Text {
            text: "IconButton (MD.Icon)"
            typescale: MD.Token.typescale.label_large
        }

        RowLayout {
            spacing: 16
            Repeater {
                model: root.artifactIcons
                delegate: ColumnLayout {
                    required property var modelData
                    spacing: 4
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label
                        typescale: MD.Token.typescale.label_small
                    }
                    MD.IconButton {
                        Layout.alignment: Qt.AlignHCenter
                        icon.name: modelData.name
                    }
                }
            }
        }

        MD.Text {
            text: "FAB Text reference (lineHeight = pixelSize)"
            typescale: MD.Token.typescale.label_large
        }

        RowLayout {
            spacing: 16
            Repeater {
                model: root.artifactIcons
                delegate: ColumnLayout {
                    required property var modelData
                    spacing: 4
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label
                        typescale: MD.Token.typescale.label_small
                    }
                    Item {
                        Layout.alignment: Qt.AlignHCenter
                        width: 40
                        height: 40
                        Text {
                            anchors.centerIn: parent
                            font.family: MD.Token.font.icon_family
                            font.pixelSize: 24
                            text: modelData.name
                            color: MD.MProp.color.on_surface
                            lineHeight: font.pixelSize
                            lineHeightMode: Text.FixedHeight
                        }
                    }
                }
            }
        }

        MD.Text {
            text: "Raw MD.Icon sizes 18 / 20 / 24"
            typescale: MD.Token.typescale.label_large
        }

        GridLayout {
            columns: root.artifactIcons.length + 1
            columnSpacing: 16
            rowSpacing: 8

            MD.Text {
                text: ""
            }
            Repeater {
                model: root.artifactIcons
                delegate: MD.Text {
                    required property var modelData
                    text: modelData.label
                    typescale: MD.Token.typescale.label_small
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            Repeater {
                model: root.iconSizes
                delegate: RowLayout {
                    required property int modelData
                    readonly property int size: modelData

                    MD.Text {
                        text: size + "px"
                        typescale: MD.Token.typescale.label_small
                    }
                    Repeater {
                        model: root.artifactIcons
                        delegate: MD.Icon {
                            required property var modelData
                            Layout.alignment: Qt.AlignHCenter
                            name: modelData.name
                            size: parent.size
                            color: MD.MProp.color.on_surface
                        }
                    }
                }
            }
        }
    }
}
