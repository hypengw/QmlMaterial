import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

// Visual regression repro for vertical-stroke glyphs at common sizes and parent scales.
Rectangle {
    id: root
    width: 560
    height: 700
    color: MD.Token.color.surface

    readonly property var artifactIcons: [
        { label: "content_copy", name: MD.Token.icon.content_copy },
        { label: "delete", name: MD.Token.icon.delete },
        { label: "videocam", name: MD.Token.icon.videocam }
    ]
    readonly property var iconSizes: [18, 20, 24]
    readonly property var parentScales: [0.3, 0.5, 1.5, 2, 3]

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

        MD.Text {
            text: "Curve-rendered MD.Icon under scaled parent (size 20)"
            typescale: MD.Token.typescale.label_large
        }

        Item {
            id: scaleGrid
            Layout.fillWidth: true
            Layout.preferredHeight: 220

            readonly property real labelWidth: 112
            readonly property real cellWidth: 72
            readonly property real rowHeight: 64

            Repeater {
                model: root.parentScales
                delegate: MD.Text {
                    required property int index
                    required property real modelData
                    x: scaleGrid.labelWidth + index * scaleGrid.cellWidth
                       + (scaleGrid.cellWidth - width) / 2
                    text: modelData + "×"
                    typescale: MD.Token.typescale.label_small
                }
            }

            Repeater {
                model: root.artifactIcons
                delegate: Item {
                    required property int index
                    required property var modelData
                    x: 0
                    y: 24 + index * scaleGrid.rowHeight
                    width: scaleGrid.width
                    height: scaleGrid.rowHeight

                    MD.Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.modelData.label
                        typescale: MD.Token.typescale.label_small
                    }

                    Repeater {
                        model: root.parentScales
                        delegate: Item {
                            required property int index
                            required property real modelData
                            x: scaleGrid.labelWidth + index * scaleGrid.cellWidth
                               + (scaleGrid.cellWidth - width) / 2
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20
                            height: 20
                            scale: modelData
                            transformOrigin: Item.Center

                            MD.Icon {
                                anchors.fill: parent
                                name: parent.parent.modelData.name
                                size: 20
                                color: MD.MProp.color.on_surface
                            }
                        }
                    }
                }
            }
        }
    }
}
