pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Control {
    id: control

    enum SelectionMode { Single, Range }

    property int selectionMode: DatePicker.SelectionMode.Single
    property date selectedDate: new Date()
    property var rangeStart
    property var rangeEnd
    property var minDate
    property var maxDate
    property int year: control.selectedDate.getFullYear()
    property int month: control.selectedDate.getMonth()
    property string supportingText: control.selectionMode === DatePicker.SelectionMode.Range
        ? "Select date range" : "Select date"
    property bool showHeader: true

    implicitWidth: 360
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding


    function _sameDay(a, b) {
        if (!a || !b) return false;
        return a.getFullYear() === b.getFullYear()
            && a.getMonth() === b.getMonth()
            && a.getDate() === b.getDate();
    }
    function _inRange(d) {
        if (control.selectionMode !== DatePicker.SelectionMode.Range) return false;
        if (!control.rangeStart || !control.rangeEnd) return false;
        const t = d.getTime();
        return t > control.rangeStart.getTime() && t < control.rangeEnd.getTime();
    }
    function _dayEnabled(d) {
        if (control.minDate && d.getTime() < control.minDate.getTime()) return false;
        if (control.maxDate && d.getTime() > control.maxDate.getTime()) return false;
        return true;
    }
    function _shiftMonth(delta) {
        let m = control.month + delta;
        let y = control.year;
        while (m < 0) { m += 12; y -= 1; }
        while (m > 11) { m -= 12; y += 1; }
        control.month = m;
        control.year = y;
    }
    function _selectDay(d) {
        if (control.selectionMode === DatePicker.SelectionMode.Single) {
            control.selectedDate = d;
        } else {
            if (!control.rangeStart || (control.rangeStart && control.rangeEnd)) {
                control.rangeStart = d;
                control.rangeEnd = undefined;
            } else if (d.getTime() < control.rangeStart.getTime()) {
                control.rangeStart = d;
            } else {
                control.rangeEnd = d;
            }
        }
    }
    function _fmtHeadline(d, withWeekday) {
        if (!d) return "—";
        return Qt.formatDate(d, withWeekday ? "ddd, MMM d" : "MMM d");
    }
    function _monthTitle() {
        return Qt.formatDate(new Date(control.year, control.month, 1), "MMMM yyyy");
    }

    background: null

    contentItem: Column {
        spacing: 0

        Item {
            id: m_header
            width: parent.width
            height: 100
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
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 24
                anchors.bottomMargin: 16
                typescale: MD.Token.typescale.headline_medium
                color: MD.MProp.color.on_surface
                text: control.selectionMode === DatePicker.SelectionMode.Single
                    ? control._fmtHeadline(control.selectedDate, true)
                    : (control._fmtHeadline(control.rangeStart, false) + "  –  " + control._fmtHeadline(control.rangeEnd, false))
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

            MD.Text {
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                text: control._monthTitle()
                typescale: MD.Token.typescale.title_small
                color: MD.MProp.color.on_surface_variant
            }

            MD.IconButton {
                anchors.right: m_nextBtn.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                icon.name: MD.Token.icon.chevron_left
                onClicked: control._shiftMonth(-1)
            }
            MD.IconButton {
                id: m_nextBtn
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                icon.name: MD.Token.icon.chevron_right
                onClicked: control._shiftMonth(1)
            }
        }

        T.AbstractDayOfWeekRow {
            id: m_dow
            anchors.horizontalCenter: parent.horizontalCenter
            width: 7 * 40 + 6 * 8
            implicitHeight: 24 + topPadding + bottomPadding
            spacing: 8
            topPadding: 4
            bottomPadding: 4
            locale: Qt.locale("en_US")

            delegate: Item {
                required property string narrowName
                implicitWidth: 40
                implicitHeight: 24
                MD.Text {
                    anchors.centerIn: parent
                    text: parent.narrowName.toUpperCase()
                    typescale: MD.Token.typescale.body_small
                    color: MD.MProp.color.on_surface_variant
                }
            }
            contentItem: Row {
                spacing: m_dow.spacing
                Repeater {
                    model: m_dow.source
                    delegate: m_dow.delegate
                }
            }
        }

        T.AbstractMonthGrid {
            id: m_grid
            anchors.horizontalCenter: parent.horizontalCenter
            width: 7 * 40 + 6 * 8
            implicitHeight: 6 * 40 + 5 * 8 + topPadding + bottomPadding
            spacing: 8
            month: control.month
            year: control.year
            locale: Qt.locale("en_US")
            onClicked: function (date) {
                if (control._dayEnabled(date)) control._selectDay(date);
            }

            delegate: Item {
                required property var model
                readonly property bool inMonth: model.month === m_grid.month
                readonly property bool selected: control.selectionMode === DatePicker.SelectionMode.Single
                    ? control._sameDay(model.date, control.selectedDate)
                    : (control._sameDay(model.date, control.rangeStart)
                        || control._sameDay(model.date, control.rangeEnd))
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
                    color: !parent.dayEnabled
                            ? MD.Util.transparent(MD.MProp.color.on_surface, 0.38)
                            : parent.selected
                                ? MD.MProp.color.on_primary
                                : parent.today
                                    ? MD.MProp.color.primary
                                    : MD.MProp.color.on_surface
                }
            }
            contentItem: Grid {
                rows: 6
                columns: 7
                rowSpacing: m_grid.spacing
                columnSpacing: m_grid.spacing
                Repeater {
                    model: m_grid.source
                    delegate: m_grid.delegate
                }
            }
        }
    }
}
