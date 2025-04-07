import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.Page {
    title: 'Components'
    padding: 0

    MD.VerticalFlickable {
        anchors.fill: parent
        topMargin: 16
        bottomMargin: 16

        ColumnLayout {
            width: parent.width

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter

                ColumnLayout {
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Actions'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'Common buttons'

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                type: MD.Enum.BtElevated
                                text: 'Elevated'
                            }
                            MD.Button {
                                type: MD.Enum.BtElevated
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                type: MD.Enum.BtElevated
                                enabled: false
                                text: 'Elevated'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                type: MD.Enum.BtFilled
                                text: 'Filled'
                            }
                            MD.Button {
                                type: MD.Enum.BtFilled
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                type: MD.Enum.BtFilled
                                enabled: false
                                text: 'Filled'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                type: MD.Enum.BtFilledTonal
                                text: 'FilledTonal'
                            }
                            MD.Button {
                                type: MD.Enum.BtFilledTonal
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                type: MD.Enum.BtFilledTonal
                                enabled: false
                                text: 'FilledTonal'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                type: MD.Enum.BtOutlined
                                text: 'Outlined'
                            }
                            MD.Button {
                                type: MD.Enum.BtOutlined
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                type: MD.Enum.BtOutlined
                                enabled: false
                                text: 'Outlined'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                type: MD.Enum.BtText
                                text: 'Text'
                            }
                            MD.Button {
                                type: MD.Enum.BtText
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                type: MD.Enum.BtText
                                enabled: false
                                text: 'Text'
                            }
                        }
                    }

                    ComponentCard {
                        title: 'Floating action buttons'
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 36

                            MD.FAB {
                                type: MD.Enum.FABSmall
                                icon.name: MD.Token.icon.add
                            }
                            MD.FAB {
                                type: MD.Enum.FABNormal
                                icon.name: MD.Token.icon.add
                            }
                            MD.FAB {
                                type: MD.Enum.FABLarge
                                icon.name: MD.Token.icon.add
                            }
                        }
                    }

                    ComponentCard {
                        title: 'Icon buttons'

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 36
                            MD.IconButton {
                                type: MD.Enum.IBtStandard
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                            MD.IconButton {
                                type: MD.Enum.IBtFilled
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                            MD.IconButton {
                                type: MD.Enum.IBtFilledTonal
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                            MD.IconButton {
                                type: MD.Enum.IBtOutlined
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 36
                            MD.IconButton {
                                type: MD.Enum.IBtStandard
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                            MD.IconButton {
                                type: MD.Enum.IBtFilled
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                            MD.IconButton {
                                type: MD.Enum.IBtFilledTonal
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                            MD.IconButton {
                                type: MD.Enum.IBtOutlined
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                        }
                    }
                }
            }

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter
                ColumnLayout {
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Communication'
                        typescale: MD.Token.typescale.title_large
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter

                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: 'Propgress indicators'
                                typescale: MD.Token.typescale.title_medium
                            }
                            MD.Pane {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 400
                                padding: 12
                                radius: MD.Token.shape.corner.medium
                                backgroundColor: MD.MatProp.color.surface_container

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 12

                                    MD.IconButton {
                                        id: m_indicator_play
                                        checkable: true
                                        icon.name: checked ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                                    }

                                    MD.CircularIndicator {
                                        running: m_indicator_play.checked
                                    }
                                }
                            }
                        }
                    }
                }
            }

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter
                ColumnLayout {
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Selection'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'Chips'
                        RowLayout {
                            spacing: 12

                            MD.AssistChip {
                                text: 'Assist'
                                icon.name: MD.Token.icon.event
                                onClicked: elevated = !elevated
                            }
                            MD.FilterChip {
                                id: m_filter_chip
                                text: 'Filter'
                            }
                            MD.InputChip {
                                text: 'Input'
                            }
                            MD.SuggestionChip {
                                text: 'Suggestion'
                            }
                        }
                        RowLayout {
                            spacing: 12

                            MD.AssistChip {
                                text: 'Assist'
                                icon.name: MD.Token.icon.event
                                enabled: false
                            }
                            MD.FilterChip {
                                text: 'Filter'
                                enabled: false
                                checked: m_filter_chip.checked
                            }
                            MD.InputChip {
                                text: 'Input'
                                enabled: false
                            }
                            MD.SuggestionChip {
                                text: 'Suggestion'
                                enabled: false
                            }
                        }
                    }
                }
            }
        }
    }

    component ComponentCard: ColumnLayout {
        id: m_comp_card
        Layout.alignment: Qt.AlignHCenter

        default property alias contentData: m_content.data
        property string title

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                text: m_comp_card.title
                typescale: MD.Token.typescale.title_medium
            }
            MD.Pane {
                Layout.alignment: Qt.AlignHCenter
                Layout.minimumWidth: 400
                padding: 12
                radius: MD.Token.shape.corner.medium
                backgroundColor: MD.MatProp.color.surface_container

                ColumnLayout {
                    id: m_content
                    anchors.fill: parent
                    spacing: 8
                }
            }
        }
    }
}
