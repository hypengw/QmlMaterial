pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.Control {
    id: control
    property MD.TimeState time: MD.TimeState {
        locale: control.locale
    }
    property bool editable: true
    signal inputRequested
    readonly property MD.time_picker_token tokens: MD.Token.time_picker
    readonly property real __fieldHeight: Math.max(tokens.fieldHeight, hours.contentHeight, minutes.contentHeight)
    readonly property real __periodWidth: Math.max(tokens.periodWidth, am.implicitContentWidth + 8, pm.implicitContentWidth + 8)
    readonly property real __supportHeight: editable ? 24 : 0
    opacity: enabled ? 1 : 0.38
    readonly property bool __stackPeriod: !time.is24Hour && availableWidth < tokens.fieldWidth * 2 + tokens.separatorWidth + __periodWidth + tokens.periodGap
    implicitWidth: tokens.fieldWidth * 2 + tokens.separatorWidth + (time.is24Hour ? 0 : tokens.periodGap + __periodWidth) + leftPadding + rightPadding
    implicitHeight: __fieldHeight + __supportHeight + (__stackPeriod ? __fieldHeight / 2 + tokens.periodGap : 0) + topPadding + bottomPadding

    function focusSelection() {
        const field = time.selection === MD.TimeState.Hours ? hours : minutes;
        field.forceActiveFocus(Qt.OtherFocusReason);
        if (editable)
            field.selectAll();
    }

    contentItem: Item {
        id: fields
        readonly property real fieldsWidth: Math.min(width, control.tokens.fieldWidth * 2 + control.tokens.separatorWidth)
        readonly property real fieldsX: control.mirrored && !control.time.is24Hour && !control.__stackPeriod ? control.__periodWidth + control.tokens.periodGap : 0
        Field {
            id: hours
            objectName: "timeHour"
            hourField: true
            x: fields.fieldsX
        }
        MD.Label {
            x: hours.x + hours.width
            width: control.tokens.separatorWidth
            height: control.__fieldHeight
            text: ":"
            typescale: MD.Token.typescale.display_large
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Field {
            id: minutes
            objectName: "timeMinute"
            hourField: false
            x: hours.x + hours.width + control.tokens.separatorWidth
        }
        Item {
            visible: !control.time.is24Hour
            width: control.__stackPeriod ? fields.fieldsWidth : control.__periodWidth
            height: control.__stackPeriod ? control.__fieldHeight / 2 : control.__fieldHeight
            x: control.__stackPeriod || control.mirrored ? 0 : fields.fieldsWidth + control.tokens.periodGap
            y: control.__stackPeriod ? control.__fieldHeight + control.__supportHeight + control.tokens.periodGap : 0
            PeriodButton {
                id: am
                period: 0
            }
            PeriodButton {
                id: pm
                x: control.__stackPeriod ? width : 0
                y: control.__stackPeriod ? 0 : height
                period: 1
            }
        }
    }
    component Field: MD.TextFieldEmbed {
        id: field
        required property bool hourField
        readonly property bool selected: control.time.selection === (hourField ? MD.TimeState.Hours : MD.TimeState.Minutes)
        readonly property bool valid: hourField ? control.time.hourAcceptable : control.time.minuteAcceptable
        width: Math.max(0, (fields.fieldsWidth - control.tokens.separatorWidth) / 2)
        height: control.__fieldHeight
        readOnly: !control.editable
        selectByMouse: control.editable
        activeFocusOnTab: true
        inputMethodHints: Qt.ImhDigitsOnly
        maximumLength: 2
        typescale: MD.Token.typescale.display_large
        horizontalAlignment: TextInput.AlignHCenter
        topPadding: 0
        bottomPadding: 0
        color: !valid ? MD.Token.color.error : selected ? MD.Token.color.on_primary_container : MD.Token.color.on_surface
        text: hourField ? control.time.hourText : control.time.minuteText
        background: Rectangle {
            radius: MD.Token.shape.corner.small
            color: field.selected ? MD.Token.color.primary_container : MD.Token.color.surface_container_highest
            border.width: !field.valid || (field.activeFocus && control.editable) ? 2 : 0
            border.color: field.valid ? MD.Token.color.primary : MD.Token.color.error
        }
        onActiveFocusChanged: if (activeFocus)
            control.time.selection = hourField ? MD.TimeState.Hours : MD.TimeState.Minutes
        onTextEdited: {
            if (hourField)
                control.time.editHour(text);
            else
                control.time.editMinute(text);
            if (hourField && valid && text.length === 2 && cursorPosition === 2) {
                control.time.selection = MD.TimeState.Minutes;
                control.focusSelection();
            }
        }
        onAccepted: {
            if (hourField && valid) {
                control.time.selection = MD.TimeState.Minutes;
                control.focusSelection();
            } else
                control.time.commitInput();
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Escape) {
                control.time.discardInput();
                event.accepted = true;
            } else if (event.key === Qt.Key_Backspace && !hourField && text.length === 0) {
                control.time.selection = MD.TimeState.Hours;
                control.focusSelection();
                event.accepted = true;
            }
        }
        TapHandler {
            enabled: !control.editable
            onTapped: control.time.selection = field.hourField ? MD.TimeState.Hours : MD.TimeState.Minutes
            onDoubleTapped: control.inputRequested()
        }
        MD.Label {
            visible: control.editable
            y: parent.height + 4
            width: parent.width
            text: field.hourField ? qsTr("Hour") : qsTr("Minute")
            typescale: MD.Token.typescale.body_small
            color: field.valid ? MD.Token.color.on_surface_variant : MD.Token.color.error
        }
    }
    component PeriodButton: MD.Button {
        required property int period
        width: control.__stackPeriod ? fields.fieldsWidth / 2 : Math.min(control.__periodWidth, control.availableWidth)
        height: control.__fieldHeight / 2
        padding: 0
        leftPadding: 4
        rightPadding: 4
        text: period === 0 ? control.time.amText : control.time.pmText
        font.capitalization: Font.MixedCase
        mdState.type: MD.Enum.BtOutlined
        mdState.corners: control.__stackPeriod ? (period === 0 ? MD.Util.corners(MD.Token.shape.corner.small, 0, MD.Token.shape.corner.small, 0) : MD.Util.corners(0, MD.Token.shape.corner.small, 0, MD.Token.shape.corner.small)) : (period === 0 ? MD.Util.corners(MD.Token.shape.corner.small, MD.Token.shape.corner.small, 0, 0) : MD.Util.corners(0, 0, MD.Token.shape.corner.small, MD.Token.shape.corner.small))
        mdState.backgroundColor: control.time.period === period ? MD.Token.color.tertiary_container : "transparent"
        mdState.textColor: control.time.period === period ? MD.Token.color.on_tertiary_container : MD.Token.color.on_surface_variant
        onClicked: control.time.period = period
    }
}
