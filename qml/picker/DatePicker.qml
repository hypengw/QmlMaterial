pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Control {
    id: control

    enum SelectionMode {
        Single,
        Range
    }

    enum DisplayMode {
        Calendar,
        Input
    }

    property int displayMode: DatePicker.DisplayMode.Calendar
    property bool showModeToggle: true
    property string inputDateFormat: ""
    readonly property MD.DatePickerState dateState: MD.DatePickerState {
        id: m_state
        locale: control.locale
        inputMode: control.displayMode === DatePicker.Input
        rules.dateFormat: control.inputDateFormat
        startInput: m_startInput
        endInput: m_endInput
    }
    property alias __navigation: m_state.navigation
    readonly property int __firstYear: dateState.firstYear
    readonly property int __lastYear: dateState.lastYear
    property alias selectionMode: m_state.selectionMode
    property alias selectedDate: m_state.selectedDate
    property alias rangeStart: m_state.rangeStart
    property alias rangeEnd: m_state.rangeEnd
    property alias minDate: m_state.minDate
    property alias maxDate: m_state.maxDate
    property alias year: m_state.year
    property alias month: m_state.month
    signal selectedDateChangeRequested(date date)
    signal rangeChangeRequested(date start, date end)
    Connections {
        target: control.dateState
        function onSelectedDateChangeRequested(date) {
            control.selectedDateChangeRequested(date);
        }
        function onRangeChangeRequested(start, end) {
            control.rangeChangeRequested(start, end);
        }
    }
    property string supportingText: control.selectionMode === DatePicker.SelectionMode.Range ? qsTr("Select date range") : qsTr("Select date")
    property bool showHeader: true
    property string headlineFormat: locale.dateFormat(Locale.ShortFormat)
    property string monthTitleFormat: "MMMM yyyy"
    readonly property bool selectionValid: dateState.selectionValid

    implicitWidth: 360
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    function _fmtHeadline(d) {
        if (!d || isNaN(d.getTime()))
            return "—";
        return d.toLocaleDateString(control.locale, control.headlineFormat);
    }
    function _monthTitle() {
        return control.dateState.calendar.firstDate.toLocaleDateString(control.locale, control.monthTitleFormat);
    }

    background: null
    contentItem: Column {
        spacing: 0

        Item {
            id: m_header
            width: parent.width
            height: Math.max(100, 16 + m_supporting.implicitHeight + 8 + m_headline.implicitHeight + 16)
            visible: control.showHeader

            MD.Text {
                id: m_supporting
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: 24
                anchors.topMargin: 16
                typescale: MD.Token.typescale.label_medium
                text: control.supportingText
                color: MD.MProp.color.on_surface_variant
            }
            MD.Text {
                id: m_headline
                anchors.right: m_modeButton.left
                anchors.rightMargin: 8
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 24
                anchors.bottomMargin: 16
                typescale: MD.Token.typescale.headline_medium
                maximumLineCount: 2
                elide: Text.ElideRight
                color: MD.MProp.color.on_surface
                text: control.selectionMode === DatePicker.SelectionMode.Single ? control._fmtHeadline(control.selectedDate) : (control._fmtHeadline(control.rangeStart) + "  –  " + control._fmtHeadline(control.rangeEnd))
            }
            MD.IconButton {
                id: m_modeButton
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 12
                visible: control.showModeToggle
                width: visible ? implicitWidth : 0
                icon.name: control.displayMode === DatePicker.DisplayMode.Calendar ? "edit" : "calendar_today"
                onClicked: control.displayMode = control.displayMode === DatePicker.DisplayMode.Calendar ? DatePicker.DisplayMode.Input : DatePicker.DisplayMode.Calendar
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: MD.MProp.color.outline_variant
            }
        }

        Item {
            width: parent.width
            height: 48
            visible: control.displayMode === DatePicker.DisplayMode.Calendar

            MD.Button {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, Math.max(0, m_previousBtn.x - x - 4))
                text: control._monthTitle()
                mdState.type: MD.Enum.BtText
                icon.name: control.__navigation ? "expand_less" : "expand_more"
                onClicked: control.dateState.toggleNavigation()
            }

            MD.IconButton {
                id: m_previousBtn
                anchors.right: m_nextBtn.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                icon.name: MD.Token.icon.chevron_left
                enabled: control.dateState.monthEnabled(control.month === 0 ? control.year - 1 : control.year, (control.month + 11) % 12)
                onClicked: control.dateState.shiftMonth(-1)
            }
            MD.IconButton {
                id: m_nextBtn
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                icon.name: MD.Token.icon.chevron_right
                enabled: control.dateState.monthEnabled(control.month === 11 ? control.year + 1 : control.year, (control.month + 1) % 12)
                onClicked: control.dateState.shiftMonth(1)
            }
        }

        GridView {
            id: m_years
            objectName: "datePickerYears"
            visible: control.displayMode === DatePicker.DisplayMode.Calendar && control.__navigation === 1
            width: parent.width
            height: 312
            clip: true
            cellWidth: width / 3
            cellHeight: 52
            model: Math.max(0, control.__lastYear - control.__firstYear + 1)
            onVisibleChanged: if (visible)
                positionViewAtIndex(Math.max(0, Math.min(count - 1, control.year - control.__firstYear)), GridView.Center)
            delegate: MD.Button {
                required property int index
                readonly property int yearValue: control.__firstYear + index
                objectName: "datePickerYear" + yearValue
                width: m_years.cellWidth
                height: m_years.cellHeight
                text: String(yearValue)
                mdState.type: yearValue === control.year ? MD.Enum.BtFilledTonal : MD.Enum.BtText
                onClicked: control.dateState.selectYear(yearValue)
            }
        }

        Grid {
            visible: control.displayMode === DatePicker.DisplayMode.Calendar && control.__navigation === 2
            width: parent.width
            columns: 3
            Repeater {
                model: control.dateState.calendar.monthNames
                delegate: MD.Button {
                    required property int index
                    required property string modelData
                    objectName: "datePickerMonth" + index
                    width: control.availableWidth / 3
                    height: 78
                    leftPadding: 8
                    rightPadding: 8
                    text: modelData
                    enabled: control.dateState.monthEnabled(control.year, index)
                    mdState.type: index === control.month ? MD.Enum.BtFilledTonal : MD.Enum.BtText
                    onClicked: control.dateState.selectMonth(index)
                }
            }
        }

        Column {
            visible: control.displayMode === DatePicker.DisplayMode.Input
            width: parent.width
            topPadding: 16
            bottomPadding: 16
            spacing: 16

            MD.TextField {
                id: m_startInput
                objectName: "datePickerStartInput"
                x: 24
                width: parent.width - 48
                placeholderText: control.selectionMode === DatePicker.SelectionMode.Single ? qsTr("Date") : qsTr("Start date")
                supportingText: control.dateState.rules.inputFormat
                validator: control.dateState.rules
                inputMethodHints: Qt.ImhDate
                selectByMouse: true
                error: text.length > 0 && !acceptableInput
                errorText: qsTr("Enter a valid date")
            }
            MD.TextField {
                id: m_endInput
                objectName: "datePickerEndInput"
                visible: control.selectionMode === DatePicker.SelectionMode.Range
                x: 24
                width: parent.width - 48
                placeholderText: qsTr("End date")
                supportingText: control.dateState.rules.inputFormat
                validator: control.dateState.rules
                inputMethodHints: Qt.ImhDate
                selectByMouse: true
                error: text.length > 0 && (!acceptableInput || (m_startInput.acceptableInput && !control.selectionValid))
                errorText: acceptableInput ? qsTr("End date must not precede start date") : qsTr("Enter a valid date")
            }
        }

        MD.Control {
            id: m_dow
            visible: control.displayMode === DatePicker.DisplayMode.Calendar && control.__navigation === 0
            anchors.horizontalCenter: parent.horizontalCenter
            width: 7 * 40 + 6 * 8
            implicitHeight: 24 + topPadding + bottomPadding
            spacing: 8
            topPadding: 4
            bottomPadding: 4

            property Component delegate: Item {
                required property var modelData
                implicitWidth: 40
                implicitHeight: 24
                MD.Text {
                    anchors.centerIn: parent
                    text: parent.modelData.narrowName.toUpperCase()
                    typescale: MD.Token.typescale.body_small
                    color: MD.MProp.color.on_surface_variant
                }
            }
            contentItem: Row {
                spacing: m_dow.spacing
                Repeater {
                    model: control.dateState.calendar.weekDays
                    delegate: m_dow.delegate
                }
            }
        }

        MD.Control {
            id: m_grid
            visible: control.displayMode === DatePicker.DisplayMode.Calendar && control.__navigation === 0
            anchors.horizontalCenter: parent.horizontalCenter
            width: 7 * 40 + 6 * 8
            implicitHeight: 6 * 40 + 5 * 8 + topPadding + bottomPadding
            spacing: 8

            property Component delegate: MD.CalendarDayBase {
                required property int index
                required property var model
                readonly property bool inMonth: model.month === control.month
                dateState: control.dateState
                dayIndex: index
                readonly property bool today: model.today
                enabled: dayEnabled
                activeFocusOnTab: tabStop || focus
                Accessible.role: Accessible.Button
                Accessible.name: date.toLocaleDateString(control.locale, Locale.LongFormat)
                Accessible.selected: selected
                Accessible.onPressAction: click()

                implicitWidth: 40
                implicitHeight: 40

                Rectangle {
                    visible: parent.inRange && !parent.selected
                    anchors.fill: parent
                    color: MD.Util.transparent(MD.MProp.color.primary, 0.16)
                }
                Rectangle {
                    id: pill
                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height) - 4
                    height: width
                    radius: width / 2
                    color: parent.selected ? MD.MProp.color.primary : "transparent"
                    border.width: (parent.today && !parent.selected) ? 1 : 0
                    border.color: MD.MProp.color.primary
                }
                MD.Text {
                    anchors.centerIn: parent
                    text: parent.model.day
                    typescale: MD.Token.typescale.body_large
                    opacity: parent.inMonth ? 1 : 0.5
                    color: !parent.dayEnabled ? MD.Util.transparent(MD.MProp.color.on_surface, 0.38) : parent.selected ? MD.MProp.color.on_primary : parent.today ? MD.MProp.color.primary : MD.MProp.color.on_surface
                }
            }
            contentItem: Grid {
                rows: 6
                columns: 7
                rowSpacing: m_grid.spacing
                columnSpacing: m_grid.spacing
                Repeater {
                    model: control.dateState.calendar
                    delegate: m_grid.delegate
                }
            }
        }
    }
}
