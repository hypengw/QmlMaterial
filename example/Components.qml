import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qt.labs.qmlmodels
import Qcm.Material as MD
import "carousel" as CarouselDemo

MD.Page {
    id: root
    title: 'Components'
    padding: 0

    CarouselDemo.CarouselDemoData {
        id: m_carousel_demo
    }

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
                                mdState.type: MD.Enum.BtElevated
                                text: 'Elevated'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtElevated
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtElevated
                                enabled: false
                                text: 'Elevated'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                text: 'Filled'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                enabled: false
                                text: 'Filled'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'FilledTonal'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtFilledTonal
                                enabled: false
                                text: 'FilledTonal'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                mdState.type: MD.Enum.BtOutlined
                                text: 'Outlined'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtOutlined
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtOutlined
                                enabled: false
                                text: 'Outlined'
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            MD.Button {
                                mdState.type: MD.Enum.BtText
                                text: 'Text'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtText
                                text: 'Icon'
                                icon.name: MD.Token.icon.add
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtText
                                enabled: false
                                text: 'Text'
                            }
                        }

                        MD.Label {
                            text: "Sizes"
                            Layout.alignment: Qt.AlignHCenter
                            typescale: MD.Token.typescale.title_medium
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 12
                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                mdState.size: MD.Enum.XS
                                text: 'XS'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                mdState.size: MD.Enum.S
                                text: 'S'
                            }
                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                mdState.size: MD.Enum.M
                                text: 'M'
                            }
                        }
                    }

                    ComponentCard {
                        title: 'Segmented buttons'

                        MD.SegmentedButtonGroup {
                            Layout.alignment: Qt.AlignHCenter
                            MD.SegmentedButton {
                                text: 'Day'
                            }
                            MD.SegmentedButton {
                                text: 'Week'
                                checked: true
                            }
                            MD.SegmentedButton {
                                text: 'Month'
                            }
                            MD.SegmentedButton {
                                text: 'Year'
                            }
                        }

                        MD.SegmentedButtonGroup {
                            Layout.alignment: Qt.AlignHCenter
                            exclusive: false
                            MD.SegmentedButton {
                                icon.name: MD.Token.icon.check
                                text: 'Option 1'
                            }
                            MD.SegmentedButton {
                                text: 'Option 2'
                            }
                            MD.SegmentedButton {
                                text: 'Option 3'
                            }
                        }

                        MD.Label {
                            text: "Sizes"
                            Layout.alignment: Qt.AlignHCenter
                            typescale: MD.Token.typescale.title_medium
                        }

                        MD.SegmentedButtonGroup {
                            Layout.alignment: Qt.AlignHCenter
                            size: MD.Enum.XS
                            MD.SegmentedButton { text: 'XS' }
                            MD.SegmentedButton { text: 'XS'; checked: true }
                            MD.SegmentedButton { text: 'XS' }
                        }
                        MD.SegmentedButtonGroup {
                            Layout.alignment: Qt.AlignHCenter
                            size: MD.Enum.S
                            MD.SegmentedButton { text: 'S' }
                            MD.SegmentedButton { text: 'S'; checked: true }
                            MD.SegmentedButton { text: 'S' }
                        }
                        MD.SegmentedButtonGroup {
                            Layout.alignment: Qt.AlignHCenter
                            size: MD.Enum.M
                            MD.SegmentedButton { text: 'M' }
                            MD.SegmentedButton { text: 'M'; checked: true }
                            MD.SegmentedButton { text: 'M' }
                        }
                    }

                    ComponentCard {
                        title: 'Split buttons'

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 16

                            MD.SplitButton {
                                text: 'Filled'
                                mdState.type: MD.Enum.BtFilled
                                menu: MD.Menu {
                                    MD.MenuItem {
                                        text: 'Action 1'
                                    }
                                    MD.MenuItem {
                                        text: 'Action 2'
                                    }
                                }
                            }

                            MD.SplitButton {
                                text: 'Tonal'
                                mdState.type: MD.Enum.BtFilledTonal
                                menu: MD.Menu {
                                    MD.MenuItem {
                                        text: 'Action A'
                                    }
                                    MD.MenuItem {
                                        text: 'Action B'
                                    }
                                }
                            }
                            MD.SplitButton {
                                text: 'Disabled'
                                mdState.type: MD.Enum.BtFilled
                                enabled:false
                                menu: MD.Menu {
                                    MD.MenuItem {
                                        text: 'Action 1'
                                    }
                                    MD.MenuItem {
                                        text: 'Action 2'
                                    }
                                }
                            }

                        }
                    }

                    ComponentCard {
                        title: 'Floating action buttons'
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 36

                            MD.FAB {
                                mdState.type: MD.Enum.FABSmall
                                icon.name: MD.Token.icon.add
                            }
                            MD.FAB {
                                mdState.type: MD.Enum.FABNormal
                                icon.name: MD.Token.icon.add
                            }
                            MD.FAB {
                                mdState.type: MD.Enum.FABLarge
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
                                mdState.type: MD.Enum.IBtStandard
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtFilled
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtFilledTonal
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtOutlined
                                icon.name: MD.Token.icon.settings
                                checkable: true
                            }
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 36
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtStandard
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtFilled
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtFilledTonal
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtOutlined
                                icon.name: MD.Token.icon.settings
                                checkable: true
                                enabled: false
                            }
                        }

                        MD.Label {
                            text: "Sizes & Width Modes"
                            Layout.alignment: Qt.AlignHCenter
                            typescale: MD.Token.typescale.title_medium
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24
                            MD.IconButton {
                                mdState.size: MD.Enum.XS
                                icon.name: MD.Token.icon.settings
                            }
                            MD.IconButton {
                                mdState.size: MD.Enum.S
                                icon.name: MD.Token.icon.settings
                            }
                            MD.IconButton {
                                mdState.size: MD.Enum.M
                                icon.name: MD.Token.icon.settings
                            }
                            MD.IconButton {
                                mdState.size: MD.Enum.L
                                icon.name: MD.Token.icon.settings
                            }
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24
                            MD.IconButton {
                                mdState.size: MD.Enum.S
                                mdState.widthMode: MD.Enum.NarrowWidth
                                icon.name: MD.Token.icon.settings
                            }
                            MD.IconButton {
                                mdState.size: MD.Enum.S
                                mdState.widthMode: MD.Enum.DefaultWidth
                                icon.name: MD.Token.icon.settings
                            }
                            MD.IconButton {
                                mdState.size: MD.Enum.S
                                mdState.widthMode: MD.Enum.WideWidth
                                icon.name: MD.Token.icon.settings
                            }
                        }

                        MD.Label {
                            text: "With Badges"
                            Layout.alignment: Qt.AlignHCenter
                            typescale: MD.Token.typescale.title_medium
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtStandard
                                icon.name: MD.Token.icon.notifications
                                MD.Badge { dot: true }
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtFilled
                                icon.name: MD.Token.icon.notifications
                                MD.Badge { count: 9 }
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtOutlined
                                icon.name: MD.Token.icon.notifications
                                MD.Badge { count: 128; maxCount: 99 }
                            }
                            MD.IconButton {
                                mdState.type: MD.Enum.IBtStandard
                                icon.name: MD.Token.icon.notifications
                                MD.Badge { text: "alarm"}
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
                    ComponentCard {
                        title: 'Loading indicators'

                        RowLayout {
                            spacing: 0
                            Item {
                                MD.IconButton {
                                    id: m_loading_indicator_play
                                    anchors.verticalCenter: parent.verticalCenter
                                    checkable: true
                                    icon.name: checked ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                implicitHeight: children[0].implicitHeight

                                MD.BusyIndicator {
                                    anchors.centerIn: parent
                                    running: m_loading_indicator_play.checked
                                }
                            }
                        }
                    }
                    ComponentCard {
                        title: 'Button indicators'

                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            MD.BusyIconButton {
                                checkable: true
                                busy: checked
                                icon.name: MD.Token.icon.add
                            }
                            MD.BusyButton {
                                anchors.verticalCenter: parent.verticalCenter
                                checkable: true
                                busy: checked
                                icon.name: MD.Token.icon.add
                                text: 'add'
                            }
                        }
                    }

                    ComponentCard {
                        title: 'SnakeBar'

                        MD.Button {
                            Layout.alignment: Qt.AlignHCenter
                            mdState.type: MD.Enum.BtText
                            text: 'Show snakeBar'
                            onClicked: {
                                m_snake.show("test");
                            }
                        }
                    }

                    ComponentCard {
                        title: 'Progress indicators'
                        spacing: 16

                        // determinate
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 8
                            MD.IconButton {
                                id: m_det_play
                                checkable: true
                                icon.name: checked ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                            }
                        }
                        MD.LinearIndicator {
                            Layout.fillWidth: true
                            indeterminate: false
                            value: m_det_play.checked ? 0.65 : 1.0
                        }
                        MD.LinearIndicator {
                            Layout.fillWidth: true
                            indeterminate: false
                            value: m_det_play.checked ? 0.65 : 1.0
                            wavy: true
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24
                            MD.CircularIndicator {
                                indeterminate: false
                                value: m_det_play.checked ? 0.65 : 1.0
                                inactiveColor: MD.MProp.color.secondary_container
                            }
                            MD.CircularIndicator {
                                indeterminate: false
                                value: m_det_play.checked ? 0.65 : 1.0
                                wavy: true
                                inactiveColor: MD.MProp.color.secondary_container
                            }
                        }

                        // indeterminate
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 8
                            MD.IconButton {
                                id: m_indet_play
                                checkable: true
                                icon.name: checked ? MD.Token.icon.pause : MD.Token.icon.play_arrow
                            }
                        }
                        MD.LinearIndicator {
                            Layout.fillWidth: true
                            indeterminate: true
                            running: m_indet_play.checked
                        }
                        MD.LinearIndicator {
                            Layout.fillWidth: true
                            indeterminate: true
                            running: m_indet_play.checked
                            wavy: true
                        }
                        MD.LinearIndicator {
                            Layout.fillWidth: true
                            indeterminate: true
                            running: m_indet_play.checked
                            type: MD.LinearIndicator.Contiguous
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24
                            MD.CircularIndicator {
                                indeterminate: true
                                running: m_indet_play.checked
                            }
                            MD.CircularIndicator {
                                indeterminate: true
                                running: m_indet_play.checked
                                wavy: true
                            }
                            MD.CircularIndicator {
                                indeterminate: true
                                running: m_indet_play.checked
                                type: MD.CircularIndicator.Reteat
                            }
                        }
                    }

                    ComponentCard {
                        title: 'Tooltips'
                        spacing: 16

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24

                            MD.Button {
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Plain'
                                MD.ToolTip {
                                    visible: parent.hovered || parent.down
                                    text: 'Add to favorites'
                                }
                            }

                            MD.Button {
                                id: m_rich_anchor
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Rich'
                                onClicked: m_rich.visible ? m_rich.close() : m_rich.open()

                                MD.RichToolTip {
                                    id: m_rich
                                    subhead: 'Rich tooltip'
                                    text: 'Rich tooltips bring attention to a particular UI element with optional title, supporting text and action.'
                                    actionItem: MD.Button {
                                        mdState.type: MD.Enum.BtText
                                        text: 'Got it'
                                        onClicked: m_rich.close()
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
                        title: 'Checkboxes'
                        spacing: 12

                        MD.CheckBox {
                            text: 'option 1'
                        }
                        MD.CheckBox {
                            text: 'option 2'
                        }
                        MD.CheckBox {
                            text: 'option 3'
                        }
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
                        title: 'Radio buttons'
                        spacing: 12

                        MD.RadioButton {
                            text: 'option 1'
                        }
                        MD.RadioButton {
                            text: 'option 2'
                        }
                        MD.RadioButton {
                            text: 'option 3'
                        }
                    }

                    ComponentCard {
                        title: 'Menus'
                        spacing: 12
                        MD.Button {
                            Layout.alignment: Qt.AlignHCenter
                            mdState.type: MD.Enum.BtText
                            text: 'Open menu'
                            onClicked: m_menu.open()
                            MD.Menu {
                                id: m_menu
                                y: parent.height
                                MD.MenuItem {
                                    text: 'First'
                                }
                                MD.MenuItem {
                                    text: 'Second'
                                }
                                MD.MenuItem {
                                    text: 'Third'
                                }
                            }
                        }
                        Row {
                            spacing: 12
                            Layout.alignment: Qt.AlignHCenter
                            MD.ComboBox {
                                model: ["First", "Second", "Third"]
                            }
                            MD.ComboBox {
                                model: ["First", "Second", "Third"]
                            }
                        }
                    }

                    ComponentCard {
                        title: 'Text Inputs'
                        spacing: 12
                        MD.TextField {
                            Layout.fillWidth: true
                            type: MD.Enum.TextFieldFilled
                            placeholderText: 'Filled'
                        }
                        MD.TextField {
                            Layout.fillWidth: true
                            leadingIcon: MD.Token.icon.search
                            trailingIcon: MD.Token.icon.info
                            type: MD.Enum.TextFieldFilled
                            placeholderText: 'Filled'
                        }

                        MD.TextField {
                            Layout.fillWidth: true
                            type: MD.Enum.TextFieldOutlined
                            placeholderText: 'Outlined'
                        }

                        MD.Label {
                            text: "Dense"
                            Layout.alignment: Qt.AlignHCenter
                            typescale: MD.Token.typescale.title_medium
                        }
                        MD.TextField {
                            Layout.fillWidth: true
                            type: MD.Enum.TextFieldFilled
                            placeholderText: 'Filled · dense'
                            mdState.dense: true
                        }
                        MD.TextField {
                            Layout.fillWidth: true
                            type: MD.Enum.TextFieldOutlined
                            placeholderText: 'Outlined · dense'
                            mdState.dense: true
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
                        MD.Switch {
                            Layout.alignment: Qt.AlignHCenter
                            checked: true
                            icon.name: checked ? MD.Token.icon.check : ''
                        }
                        MD.Switch {
                            Layout.alignment: Qt.AlignHCenter
                            icon.name: checked ? MD.Token.icon.check : MD.Token.icon.close
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
                        text: 'Dialogs'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        id: m_fd_card
                        title: 'File pickers'
                        spacing: 12

                        property string lastResult: ''

                        MD.FileDialog {
                            id: m_fd_open_one
                            title: 'Open file'
                            fileMode: MD.FileDialog.OpenFile
                            nameFilters: ['Images (*.png *.jpg *.jpeg)', 'Text (*.txt *.md)', 'All files (*)']
                            onAccepted: m_fd_card.lastResult = 'OpenFile: ' + selectedFile
                            onRejected: m_fd_card.lastResult = 'OpenFile: cancelled'
                        }
                        MD.FileDialog {
                            id: m_fd_open_many
                            title: 'Open files'
                            fileMode: MD.FileDialog.OpenFiles
                            nameFilters: ['All files (*)']
                            onAccepted: m_fd_card.lastResult = 'OpenFiles: ' + JSON.stringify(selectedFiles.map(u => u.toString()))
                            onRejected: m_fd_card.lastResult = 'OpenFiles: cancelled'
                        }
                        MD.FileDialog {
                            id: m_fd_save
                            title: 'Save file'
                            fileMode: MD.FileDialog.SaveFile
                            nameFilters: ['Text (*.txt)', 'All files (*)']
                            defaultSuffix: 'txt'
                            onAccepted: m_fd_card.lastResult = 'SaveFile: ' + selectedFile
                            onRejected: m_fd_card.lastResult = 'SaveFile: cancelled'
                        }
                        MD.FolderDialog {
                            id: m_fd_folder
                            title: 'Pick folder'
                            onAccepted: m_fd_card.lastResult = 'FolderDialog: ' + selectedFolder
                            onRejected: m_fd_card.lastResult = 'FolderDialog: cancelled'
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 8
                            columnSpacing: 8

                            MD.Button {
                                Layout.fillWidth: true
                                mdState.type: MD.Enum.BtFilled
                                text: 'Open file'
                                icon.name: MD.Token.icon.file_open
                                onClicked: m_fd_open_one.open()
                            }
                            MD.Button {
                                Layout.fillWidth: true
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Open multiple'
                                icon.name: MD.Token.icon.file_open
                                onClicked: m_fd_open_many.open()
                            }
                            MD.Button {
                                Layout.fillWidth: true
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Save file'
                                icon.name: MD.Token.icon.file_save
                                onClicked: m_fd_save.open()
                            }
                            MD.Button {
                                Layout.fillWidth: true
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Pick folder'
                                icon.name: MD.Token.icon.folder
                                onClicked: m_fd_folder.open()
                            }
                        }

                        MD.Label {
                            Layout.fillWidth: true
                            visible: m_fd_card.lastResult.length > 0
                            text: m_fd_card.lastResult
                            typescale: MD.Token.typescale.body_small
                            opacity: 0.8
                            wrapMode: Text.Wrap
                        }
                    }

                    ComponentCard {
                        id: m_bottom_sheet_card
                        title: 'Bottom sheets'
                        spacing: 12

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 12

                            MD.Button {
                                mdState.type: MD.Enum.BtFilled
                                text: 'Modal'
                                onClicked: {
                                    m_standard_sheet.close();
                                    m_modal_sheet.open();
                                }
                            }

                            MD.Button {
                                mdState.type: MD.Enum.BtFilledTonal
                                text: 'Standard'
                                onClicked: {
                                    m_modal_sheet.close();
                                    m_standard_sheet.open();
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
                        text: 'Navigation'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'Navigation Bar'
                        MD.Pane {
                            id: m_nav_bar
                            Layout.fillWidth: true
                            padding: 0
                            backgroundColor: MD.MProp.color.surface_container
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
                            mdState.backgroundColor: MD.MProp.color.surface_container_highest
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
                            mdState.backgroundColor: MD.MProp.color.surface_container_highest
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
                            mdState.backgroundColor: MD.MProp.color.surface_container_highest
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
                            mdState.backgroundColor: MD.MProp.color.surface_container_highest
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

            MD.Pane {
                Layout.alignment: Qt.AlignHCenter
                ColumnLayout {
                    spacing: 16
                    MD.Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: 'Sliders'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'M3'
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 24

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                MD.Text {
                                    text: 'Continuous'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                }

                                MD.Text {
                                    text: 'No value / stop indicators'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    labelBehavior: MD.Enum.SliderLabelGone
                                    tickVisibilityMode: MD.Enum.SliderTickNone
                                }

                                MD.Text {
                                    text: 'Discrete snap, no indicators'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    snapMode: T.Slider.SnapAlways
                                    from: 0
                                    stepSize: 20
                                    to: 100
                                    labelBehavior: MD.Enum.SliderLabelGone
                                    tickVisibilityMode: MD.Enum.SliderTickNone
                                }

                                MD.Text {
                                    text: 'Discrete + stops'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    snapMode: T.Slider.SnapAlways
                                    from: 0
                                    stepSize: 20
                                    to: 100
                                }

                                MD.Text {
                                    text: 'Snap without stops'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    snapMode: T.Slider.SnapAlways
                                    from: 0
                                    stepSize: 20
                                    to: 100
                                    tickVisibilityMode: MD.Enum.SliderTickNone
                                }

                                MD.Text {
                                    text: 'Value label always'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    value: 50
                                    labelBehavior: MD.Enum.SliderLabelVisible
                                }
                                MD.Text {
                                    text: 'Inset icon'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.Slider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    value: 50
                                    sliderSize: MD.Enum.SliderSizeMedium
                                    insetIcon: MD.Token.icon.volume_up
                                    insetIconAtMin: MD.Token.icon.volume_off
                                    labelBehavior: MD.Enum.SliderLabelGone
                                    tickVisibilityMode: MD.Enum.SliderTickNone
                                }
                            }

                            GridLayout {
                                Layout.alignment: Qt.AlignTop
                                columns: 2
                                rowSpacing: 16
                                columnSpacing: 24

                                ColumnLayout {
                                    spacing: 8

                                    MD.Text {
                                        text: 'Continuous'
                                        typescale: MD.Token.typescale.label_medium
                                        opacity: 0.8
                                    }
                                    MD.Slider {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredHeight: 160
                                        orientation: Qt.Vertical
                                        from: 0
                                        to: 100
                                    }
                                }

                                ColumnLayout {
                                    spacing: 8

                                    MD.Text {
                                        text: 'Discrete + stops'
                                        typescale: MD.Token.typescale.label_medium
                                        opacity: 0.8
                                    }
                                    MD.Slider {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredHeight: 160
                                        orientation: Qt.Vertical
                                        snapMode: T.Slider.SnapAlways
                                        from: 0
                                        stepSize: 20
                                        to: 100
                                    }
                                }

                                ColumnLayout {
                                    spacing: 8

                                    MD.Text {
                                        text: 'Snap without stops'
                                        typescale: MD.Token.typescale.label_medium
                                        opacity: 0.8
                                    }
                                    MD.Slider {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredHeight: 160
                                        orientation: Qt.Vertical
                                        snapMode: T.Slider.SnapAlways
                                        from: 0
                                        stepSize: 20
                                        to: 100
                                        tickVisibilityMode: MD.Enum.SliderTickNone
                                    }
                                }

                                ColumnLayout {
                                    spacing: 8

                                    MD.Text {
                                        text: 'Inset icon'
                                        typescale: MD.Token.typescale.label_medium
                                        opacity: 0.8
                                    }
                                    MD.Slider {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredHeight: 240
                                        orientation: Qt.Vertical
                                        from: 0
                                        to: 100
                                        value: 60
                                        sliderSize: MD.Enum.SliderSizeMedium
                                        insetIcon: MD.Token.icon.music_note
                                        insetIconAtMin: MD.Token.icon.volume_off
                                        labelBehavior: MD.Enum.SliderLabelGone
                                        tickVisibilityMode: MD.Enum.SliderTickNone
                                    }
                                }
                            }
                        }
                    }

                    ComponentCard {
                        title: 'M2'
                        spacing: 12

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 24

                            ColumnLayout {
                                spacing: 8

                                MD.Text {
                                    text: 'Continuous'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.SliderM2 {
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                MD.Text {
                                    text: 'Discrete'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.SliderM2 {
                                    Layout.alignment: Qt.AlignHCenter
                                    snapMode: T.Slider.SnapAlways
                                    from: 0
                                    stepSize: 20
                                    to: 100
                                }
                            }

                            ColumnLayout {
                                spacing: 8

                                MD.Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: 'Vertical'
                                    typescale: MD.Token.typescale.label_medium
                                    opacity: 0.8
                                }
                                MD.SliderM2 {
                                    Layout.alignment: Qt.AlignHCenter
                                    orientation: Qt.Vertical
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
                        text: 'Carousel'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'Uncontained'
                        spacing: 12

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Fixed width'
                            typescale: MD.Token.typescale.label_medium
                            opacity: 0.8
                        }
                        MD.Carousel {
                            Layout.preferredWidth: 480
                            layout: MD.Enum.CarouselUncontained
                            header: 'Photo strip'
                            itemExtent: 180
                            model: m_carousel_demo.model
                            delegate: MD.CarouselImageDelegate {}
                        }

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Multi-aspect'
                            typescale: MD.Token.typescale.label_medium
                            opacity: 0.8
                        }
                        MD.Carousel {
                            Layout.preferredWidth: 480
                            layout: MD.Enum.CarouselUncontainedMultiAspect
                            model: m_carousel_demo.model
                            delegate: MD.CarouselImageDelegate {}
                        }
                    }

                    ComponentCard {
                        title: 'Multi-browse'

                        MD.Carousel {
                            Layout.preferredWidth: 480
                            layout: MD.Enum.CarouselMultiBrowse
                            header: 'Photo gallery'
                            model: m_carousel_demo.model
                            delegate: MD.CarouselImageDelegate {}
                            showPageIndicator: true
                            showNavigationButtons: true
                        }
                    }

                    ComponentCard {
                        title: 'Hero'
                        spacing: 12

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Start-aligned'
                            typescale: MD.Token.typescale.label_medium
                            opacity: 0.8
                        }
                        MD.Carousel {
                            Layout.preferredWidth: 480
                            Layout.preferredHeight: 240
                            layout: MD.Enum.CarouselHero
                            header: 'Featured photos'
                            model: m_carousel_demo.model
                            delegate: MD.CarouselImageDelegate {}
                            showNavigationButtons: true
                        }

                        MD.Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: 'Center-aligned'
                            typescale: MD.Token.typescale.label_medium
                            opacity: 0.8
                        }
                        MD.Carousel {
                            Layout.preferredWidth: 480
                            Layout.preferredHeight: 240
                            layout: MD.Enum.CarouselHeroCenter
                            model: m_carousel_demo.model
                            delegate: MD.CarouselImageDelegate {}
                            showNavigationButtons: true
                        }
                    }

                    ComponentCard {
                        title: 'Full-screen (vertical)'

                        MD.Carousel {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 420
                            layout: MD.Enum.CarouselFullScreen
                            header: 'Full-screen gallery'
                            orientation: Qt.Vertical
                            model: m_carousel_demo.model
                            delegate: MD.CarouselImageDelegate {}
                            showNavigationButtons: true
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
                        text: 'Pickers'
                        typescale: MD.Token.typescale.title_large
                    }

                    ComponentCard {
                        title: 'Date picker'

                        MD.DateTextField {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 200
                            value: m_date_picker.selectedDate
                            onModified: function (d) { m_date_picker.selectedDate = d; }
                        }
                        MD.Divider {
                            Layout.fillWidth: true
                        }
                        MD.DatePicker {
                            id: m_date_picker
                            Layout.alignment: Qt.AlignHCenter
                            showHeader: false
                            selectionMode: MD.DatePicker.SelectionMode.Single
                            selectedDate: new Date()
                        }
                    }

                    ComponentCard {
                        title: 'Color picker'

                        MD.ColorPickerButton {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 120
                            color: m_color_picker.color
                            onAccepted: function (c) { m_color_picker.color = c; }
                        }
                        MD.Divider {
                            Layout.fillWidth: true
                        }
                        MD.ColorPicker {
                            id: m_color_picker
                            Layout.fillWidth: true
                            Layout.preferredWidth: 400
                            showHeader: false
                            color: '#6750A4'
                        }
                    }

                    ComponentCard {
                        title: 'Table'

                        TableModel {
                            id: m_table_model

                            TableModelColumn { display: "component" }
                            TableModelColumn { display: "usage" }

                            rows: [
                                { component: "Buttons", usage: "Primary and secondary actions" },
                                { component: "Bottom sheets", usage: "Modal and persistent surfaces" },
                                { component: "Navigation", usage: "Destination switching" },
                                { component: "Text inputs", usage: "Form controls" },
                                { component: "Tables", usage: "Structured data" }
                            ]
                        }

                        GridLayout {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 560
                            Layout.preferredHeight: 268
                            columns: 1
                            columnSpacing: 0
                            rowSpacing: 0

                            MD.HorizontalHeaderView {
                                id: m_horizontal_header
                                Layout.row: 0
                                Layout.column: 0
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                clip: true
                                syncView: m_table_view
                                model: ["Component", "Usage"]
                            }

                            MD.TableView {
                                id: m_table_view
                                Layout.row: 1
                                Layout.column: 0
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: MD.Token.shape.corner.extra_large
                                hasHeader: true
                                model: m_table_model
                                selectionBehavior: TableView.SelectionDisabled
                                columnWidthProvider: function(column) {
                                    return [200, 360][column] ?? 120;
                                }
                                rowHeightProvider: function() {
                                    return 44;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    MD.BottomSheet {
        id: m_modal_sheet
        anchors.fill: parent
        sheetType: MD.Enum.BottomSheetModal

        ColumnLayout {
            width: m_modal_sheet.sheetWidth
            spacing: 0

            MD.SheetActionBar {
                Layout.fillWidth: true
                actions: [
                    MD.Action {
                        text: 'Share'
                        icon.name: MD.Token.icon.share
                    },
                    MD.Action {
                        text: 'Add to'
                        icon.name: MD.Token.icon.add
                    },
                    MD.Action {
                        text: 'Trash'
                        icon.name: MD.Token.icon.delete_forever
                    },
                    MD.Action {
                        text: 'Archive'
                        icon.name: MD.Token.icon.archive
                    }
                ]
            }

            MD.Divider {
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 24
                Layout.bottomMargin: 24

                MD.Button {
                    mdState.type: MD.Enum.BtText
                    text: 'Close'
                    onClicked: m_modal_sheet.close()
                }
            }
        }
    }

    MD.BottomSheet {
        id: m_standard_sheet
        anchors.fill: parent
        sheetType: MD.Enum.BottomSheetStandard
        dim: false
        lowHeight: 176

        ColumnLayout {
            width: m_standard_sheet.sheetWidth
            spacing: 0

            MD.Label {
                Layout.fillWidth: true
                Layout.margins: 24
                text: 'Standard bottom sheet'
                typescale: MD.Token.typescale.title_medium
            }

            Repeater {
                model: [
                    [MD.Token.icon.folder, 'Project files'],
                    [MD.Token.icon.schedule, 'Recent activity'],
                    [MD.Token.icon.notifications, 'Notifications'],
                    [MD.Token.icon.settings, 'Sheet settings']
                ]

                RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: 24
                    Layout.rightMargin: 24
                    Layout.preferredHeight: 56
                    spacing: 16

                    MD.Icon {
                        name: modelData[0]
                        size: 24
                        color: MD.MProp.color.on_surface_variant
                    }

                    MD.Label {
                        Layout.fillWidth: true
                        text: modelData[1]
                        typescale: MD.Token.typescale.body_large
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 24
                Layout.bottomMargin: 24

                MD.Button {
                    mdState.type: MD.Enum.BtText
                    text: 'Close'
                    onClicked: m_standard_sheet.close()
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
                backgroundColor: MD.MProp.color.surface_container

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
