import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
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

                                ColumnLayout {
                                    anchors.fill: parent
                                    RowLayout {
                                        spacing: 0
                                        Item {
                                            MD.IconButton {
                                                id: m_indicator_play
                                                anchors.verticalCenter: parent.verticalCenter
                                                checkable: true
                                                icon.name: checked ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                                            }
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                            implicitHeight: children[0].implicitHeight
                                            MD.CircularIndicator {
                                                anchors.centerIn: parent
                                                running: m_indicator_play.checked
                                            }
                                        }
                                    }
                                    RowLayout {
                                        spacing: 0
                                        MD.IconButton {
                                            id: m_linear_indicator_play
                                            checkable: true
                                            icon.name: checked ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                                        }

                                        MD.LinearIndicator {
                                            Layout.fillWidth: true
                                            Layout.alignment: Qt.AlignVCenter
                                            running: m_linear_indicator_play.checked
                                        }
                                    }
                                }
                            }
                        }
                    }
                    ComponentCard {
                        title: 'SnakeBar'

                        MD.Button {
                            Layout.alignment: Qt.AlignHCenter
                            type: MD.Enum.BtText
                            text: 'Show snakeBar'
                            onClicked: {
                                m_snake.show("test");
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

                    ComponentCard {
                        title: 'Text Inputs'
                        MD.TextField {
                            Layout.fillWidth: true
                            type: MD.Enum.TextFieldFilled
                            placeholderText: 'Filled'
                        }
                        MD.TextField {
                            Layout.fillWidth: true
                            type: MD.Enum.TextFieldOutlined
                            placeholderText: 'Outlined'
                        }
                    }
                    ComponentCard {
                        title: 'Sliders'
                        MD.Slider {
                            Layout.alignment: Qt.AlignHCenter
                        }
                        MD.Slider {
                            Layout.alignment: Qt.AlignHCenter
                            snapMode: T.Slider.SnapAlways
                            from: 0
                            stepSize: 20
                            to: 100
                        }
                    }
                    ComponentCard {
                        title: 'Switches'
                        MD.Switch {
                            Layout.alignment: Qt.AlignHCenter
                        }
                        MD.Switch {
                            Layout.alignment: Qt.AlignHCenter
                            enabled: false
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
                        text: 'Navigation'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'Navigation Bar'
                        MD.Pane {
                            id: m_nav_bar
                            Layout.fillWidth: true
                            padding: 0
                            backgroundColor: MD.MatProp.color.surface_container
                            property int idx: 1
                            RowLayout {
                                anchors.fill: parent
                                MD.BarItem {
                                    Layout.alignment: Qt.AlignHCenter
                                    icon.name: MD.Token.icon.explore
                                    text: 'explore'
                                    checked: m_nav_bar.idx == 1
                                    onClicked: {
                                        m_nav_bar.idx = 1;
                                    }
                                }
                                MD.BarItem {
                                    Layout.alignment: Qt.AlignHCenter
                                    icon.name: MD.Token.icon.pets
                                    text: 'pets'
                                    checked: m_nav_bar.idx == 2
                                    onClicked: {
                                        m_nav_bar.idx = 2;
                                    }
                                }
                                MD.BarItem {
                                    Layout.alignment: Qt.AlignHCenter
                                    icon.name: MD.Token.icon.person
                                    text: 'account'
                                    checked: m_nav_bar.idx == 3
                                    onClicked: {
                                        m_nav_bar.idx = 3;
                                    }
                                }
                            }
                        }
                    }
                    ComponentCard {
                        title: 'Tabs'
                        MD.TabBar {
                            Layout.fillWidth: true
                            MD.TabButton {
                                text: 'Video'
                            }
                            MD.TabButton {
                                text: 'Photos'
                            }
                            MD.TabButton {
                                text: 'Audio'
                            }
                        }
                    }
                    ComponentCard {
                        title: 'Top app bars'
                        MD.AppBar {
                            Layout.fillWidth: true
                            title: 'Center-aligned'
                            mdState.backgroundColor: MD.MatProp.color.surface_container_highest
                            leadingAction: MD.Action {
                                icon.name: MD.Token.icon.arrow_back
                            }
                            actions: [
                                MD.Action {
                                    icon.name: MD.Token.icon.more_vert
                                }
                            ]
                        }
                        MD.AppBar {
                            Layout.fillWidth: true
                            type: MD.Enum.AppBarSmall
                            title: 'Small'
                            mdState.backgroundColor: MD.MatProp.color.surface_container_highest
                            leadingAction: MD.Action {
                                icon.name: MD.Token.icon.arrow_back
                            }
                            actions: [
                                MD.Action {
                                    icon.name: MD.Token.icon.more_vert
                                }
                            ]
                        }
                        MD.AppBar {
                            id: m_app_bar_1
                            Layout.fillWidth: true
                            type: MD.Enum.AppBarMedium
                            title: 'Medium'
                            mdState.backgroundColor: MD.MatProp.color.surface_container_highest
                            leadingAction: MD.Action {
                                icon.name: MD.Token.icon.arrow_back
                            }
                            actions: [
                                MD.Action {
                                    icon.name: MD.Token.icon.more_vert
                                }
                            ]
                        }
                        MD.AppBar {
                            Layout.fillWidth: true
                            type: MD.Enum.AppBarLarge
                            title: 'Large'
                            mdState.backgroundColor: MD.MatProp.color.surface_container_highest
                            leadingAction: MD.Action {
                                icon.name: MD.Token.icon.arrow_back
                            }
                            actions: [
                                MD.Action {
                                    icon.name: MD.Token.icon.more_vert
                                }
                            ]
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

    MD.SnakeView {
        id: m_snake
        parent: T.Overlay.overlay
        anchors.fill: parent
        MD.InputBlock {
            target: m_snake
        }
    }
}
