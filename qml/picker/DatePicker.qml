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
    property int __navigation: 0
    property bool __editingInput: false
    property bool __complete: false
    readonly property int __firstYear: Math.max(1, minDate && !isNaN(minDate.getTime()) ? minDate.getFullYear() : 1)
    readonly property int __lastYear: Math.min(9999, maxDate && !isNaN(maxDate.getTime()) ? maxDate.getFullYear() : 9999)

    property int selectionMode: DatePicker.SelectionMode.Single
    property date selectedDate: new Date()
    property var rangeStart
    property var rangeEnd
    property var minDate
    property var maxDate
    property int year: isNaN(selectedDate.getTime()) ? new Date().getFullYear() : selectedDate.getFullYear()
    property int month: isNaN(selectedDate.getTime()) ? new Date().getMonth() : selectedDate.getMonth()
    property string supportingText: control.selectionMode === DatePicker.SelectionMode.Range ? qsTr("Select date range") : qsTr("Select date")
    property bool showHeader: true
    property string headlineFormat: locale.dateFormat(Locale.ShortFormat)
    property string monthTitleFormat: "MMMM yyyy"
    readonly property date __invalidDate: new Date(NaN)
    readonly property bool selectionValid: selectionMode === DatePicker.SelectionMode.Single ? _dayEnabled(selectedDate) : m_rules.rangeValid(rangeStart ?? __invalidDate, rangeEnd ?? __invalidDate, minDate ?? __invalidDate, maxDate ?? __invalidDate)

    implicitWidth: 360
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    function _syncInputs() {
        if (!__complete || __editingInput)
            return;
        m_startInput.text = m_rules.formatDate(selectionMode === DatePicker.SelectionMode.Single ? selectedDate : rangeStart ?? __invalidDate);
        m_endInput.text = m_rules.formatDate(rangeEnd ?? __invalidDate);
    }
    function _editInput(end, text) {
        __editingInput = true;
        try {
            const date = m_rules.parse(text);
            if (selectionMode === DatePicker.SelectionMode.Single)
                selectedDate = date;
            else if (end)
                rangeEnd = isNaN(date.getTime()) ? undefined : date;
            else
                rangeStart = isNaN(date.getTime()) ? undefined : date;
        } finally {
            __editingInput = false;
        }
    }
    function _monthEnabled(y, m) {
        return m_rules.monthEnabled(y, m, minDate ?? __invalidDate, maxDate ?? __invalidDate);
    }
    onSelectedDateChanged: _syncInputs()
    onRangeStartChanged: _syncInputs()
    onRangeEndChanged: _syncInputs()
    onSelectionModeChanged: _syncInputs()
    onDisplayModeChanged: {
        __navigation = 0;
        _syncInputs();
        const date = selectionMode === DatePicker.SelectionMode.Single ? selectedDate : rangeStart;
        if (date && !isNaN(date.getTime())) {
            year = date.getFullYear();
            month = date.getMonth();
        }
        if (__complete) {
            if (displayMode === DatePicker.DisplayMode.Input)
                m_startInput.forceActiveFocus();
            else
                m_grid.forceActiveFocus();
        }
    }
    Component.onCompleted: {
        __complete = true;
        _syncInputs();
    }

    function _sameDay(a, b) {
        return m_rules.sameDay(a ?? __invalidDate, b ?? __invalidDate);
    }
    function _inRange(d) {
        if (control.selectionMode !== DatePicker.SelectionMode.Range)
            return false;
        return m_rules.insideRange(d, control.rangeStart ?? __invalidDate, control.rangeEnd ?? __invalidDate);
    }
    function _dayEnabled(d) {
        return m_rules.dateEnabled(d ?? __invalidDate, control.minDate ?? __invalidDate, control.maxDate ?? __invalidDate);
    }
    function _shiftMonth(delta) {
        let m = control.month + delta;
        let y = control.year;
        while (m < 0) {
            m += 12;
            y -= 1;
        }
        while (m > 11) {
            m -= 12;
            y += 1;
        }
        control.month = m;
        control.year = y;
    }
    function _selectDay(d) {
        if (!_dayEnabled(d))
            return;
        if (control.selectionMode === DatePicker.SelectionMode.Single) {
            control.selectedDate = d;
        } else {
            if (!control.rangeStart || isNaN(control.rangeStart.getTime()) || (control.rangeEnd && !isNaN(control.rangeEnd.getTime()))) {
                control.rangeStart = d;
                control.rangeEnd = undefined;
            } else if (!m_rules.rangeValid(control.rangeStart, d, control.minDate ?? __invalidDate, control.maxDate ?? __invalidDate)) {
                control.rangeStart = d;
            } else {
                control.rangeEnd = d;
            }
        }
    }
    function _fmtHeadline(d) {
        if (!d || isNaN(d.getTime()))
            return "—";
        return d.toLocaleDateString(control.locale, control.headlineFormat);
    }
    function _monthTitle() {
        return m_calendar.firstDate.toLocaleDateString(control.locale, control.monthTitleFormat);
    }

    background: null
    MD.DateValidator {
        id: m_rules
        locale: control.locale
        dateFormat: control.inputDateFormat
        minDate: control.minDate ?? control.__invalidDate
        maxDate: control.maxDate ?? control.__invalidDate
        onInputFormatChanged: control._syncInputs()
    }

    MD.CalendarMonthModel {
        id: m_calendar
        month: control.month
        year: control.year
        locale: control.locale
    }

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
                onClicked: control.__navigation = control.__navigation ? 0 : 1
            }

            MD.IconButton {
                id: m_previousBtn
                anchors.right: m_nextBtn.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                icon.name: MD.Token.icon.chevron_left
                enabled: control._monthEnabled(control.month === 0 ? control.year - 1 : control.year, (control.month + 11) % 12)
                onClicked: control._shiftMonth(-1)
            }
            MD.IconButton {
                id: m_nextBtn
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                icon.name: MD.Token.icon.chevron_right
                enabled: control._monthEnabled(control.month === 11 ? control.year + 1 : control.year, (control.month + 1) % 12)
                onClicked: control._shiftMonth(1)
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
                onClicked: {
                    control.year = yearValue;
                    control.__navigation = 2;
                }
            }
        }

        Grid {
            visible: control.displayMode === DatePicker.DisplayMode.Calendar && control.__navigation === 2
            width: parent.width
            columns: 3
            Repeater {
                model: m_calendar.monthNames
                delegate: MD.Button {
                    required property int index
                    required property string modelData
                    objectName: "datePickerMonth" + index
                    width: control.availableWidth / 3
                    height: 78
                    leftPadding: 8
                    rightPadding: 8
                    text: modelData
                    enabled: control._monthEnabled(control.year, index)
                    mdState.type: index === control.month ? MD.Enum.BtFilledTonal : MD.Enum.BtText
                    onClicked: {
                        control.month = index;
                        control.__navigation = 0;
                    }
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
                supportingText: m_rules.inputFormat
                validator: m_rules
                inputMethodHints: Qt.ImhDate
                selectByMouse: true
                error: text.length > 0 && !acceptableInput
                errorText: qsTr("Enter a valid date")
                onTextEdited: control._editInput(false, text)
            }
            MD.TextField {
                id: m_endInput
                objectName: "datePickerEndInput"
                visible: control.selectionMode === DatePicker.SelectionMode.Range
                x: 24
                width: parent.width - 48
                placeholderText: qsTr("End date")
                supportingText: m_rules.inputFormat
                validator: m_rules
                inputMethodHints: Qt.ImhDate
                selectByMouse: true
                error: text.length > 0 && (!acceptableInput || (m_startInput.acceptableInput && !control.selectionValid))
                errorText: acceptableInput ? qsTr("End date must not precede start date") : qsTr("Enter a valid date")
                onTextEdited: control._editInput(true, text)
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
                    model: m_calendar.weekDays
                    delegate: m_dow.delegate
                }
            }
        }

        MD.Control {
            id: m_grid
            visible: control.displayMode === DatePicker.DisplayMode.Calendar && control.__navigation === 0
            activeFocusOnTab: true
            anchors.horizontalCenter: parent.horizontalCenter
            width: 7 * 40 + 6 * 8
            implicitHeight: 6 * 40 + 5 * 8 + topPadding + bottomPadding
            spacing: 8

            property Component delegate: Item {
                required property int index
                required property var model
                readonly property bool inMonth: model.month === control.month
                readonly property bool selected: control.selectionMode === DatePicker.SelectionMode.Single ? control._sameDay(model.date, control.selectedDate) : (control._sameDay(model.date, control.rangeStart) || control._sameDay(model.date, control.rangeEnd))
                readonly property bool inRange: control._inRange(model.date)
                readonly property bool today: model.today
                readonly property bool dayEnabled: control._dayEnabled(model.date)

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
                    model: m_calendar
                    delegate: m_grid.delegate
                }
            }

            MouseArea {
                id: m_pointer
                anchors.fill: m_grid.contentItem
                acceptedButtons: Qt.LeftButton
                property bool tracking: false

                function indexAt(x, y) {
                    const cell = m_grid.contentItem.childAt(x, y);
                    return cell ? cell.index : -1;
                }
                onPressed: tracking = true
                onReleased: function (mouse) {
                    const index = tracking ? indexAt(mouse.x, mouse.y) : -1;
                    tracking = false;
                    if (index >= 0) {
                        const date = m_calendar.dateAt(index);
                        if (control._dayEnabled(date))
                            control._selectDay(date);
                    }
                }
                onCanceled: {
                    tracking = false;
                }
            }
            Connections {
                target: m_calendar
                function onDataChanged() {
                    m_pointer.tracking = false;
                }
            }
        }
    }
}
