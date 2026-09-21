import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

Rectangle {
    id: root
    width: 800
    height: 600
    color: MD.Token.color.surface

    Grid {
        anchors.fill: parent
        anchors.margins: 32
        columns: 4
        spacing: 8

        Repeater {
            model: 16

            Rectangle {
                required property int index
                width: 176
                height: 112
                radius: 8
                color: Qt.hsla(index / 16, 0.35, 0.72, 1)
            }
        }
    }

    Item {
        x: 96
        y: 120
        width: 360
        height: 240

        MD.BottomSheet {
            id: sheet
            sheetType: MD.Enum.BottomSheetModal
            lowHeight: 184

            Component.onCompleted: open()

            ColumnLayout {
                width: sheet.sheetWidth
                spacing: 0

                MD.SheetActionBar {
                    Layout.fillWidth: true
                    actions: [
                        MD.Action {
                            text: "Share"
                            icon.name: MD.Token.icon.share
                        },
                        MD.Action {
                            text: "Add to"
                            icon.name: MD.Token.icon.add
                        },
                        MD.Action {
                            text: "Trash"
                            icon.name: MD.Token.icon.delete_forever
                        },
                        MD.Action {
                            text: "Order"
                            icon.name: MD.Token.icon.shopping_bag
                        },
                        MD.Action {
                            text: "Archive"
                            icon.name: MD.Token.icon.archive
                        }
                    ]
                }

                MD.Divider {
                    Layout.fillWidth: true
                    Layout.leftMargin: 32
                    Layout.rightMargin: 32
                }

                MD.Label {
                    Layout.topMargin: 24
                    Layout.leftMargin: 32
                    text: "Send"
                    typescale: MD.Token.typescale.title_large
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 32
                    Layout.rightMargin: 32
                    Layout.topMargin: 20
                    Layout.bottomMargin: 32
                    spacing: 24

                    Repeater {
                        model: [
                            "Alejandro",
                            "Oli Ortega",
                            "Carmen",
                            "Ana Russo",
                            "Marty Reyes"
                        ]

                        ColumnLayout {
                            required property string modelData
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                width: 56
                                height: 56
                                radius: 28
                                color: MD.Token.color.secondary_container
                            }

                            MD.Label {
                                Layout.alignment: Qt.AlignHCenter
                                horizontalAlignment: Text.AlignHCenter
                                text: modelData
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }
    }
}
