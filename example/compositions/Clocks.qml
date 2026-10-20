pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: 376
    implicitHeight: content.implicitHeight
    property int presetIndex: 0
    property bool fixedTime: false
    property bool seconds: true
    property bool transparentStyle: false
    property bool showAlarm: true
    property MD.TimeState alarmTime: MD.TimeState {
        hour: 7
        minute: 30
        hourFormat: root.formats[root.formatIndex]
    }
    property bool date: true
    property bool period: true
    property int formatIndex: 0
    property int shapeIndex: 0
    readonly property var presets: [ClockWidget.NumeralStyle, ClockWidget.ScallopStyle, ClockWidget.DayStyle, ClockWidget.CloverStyle, ClockWidget.DigitalStyle, ClockWidget.StackedStyle]
    readonly property var presetNames: ['Numeral', 'Scallop', 'Day', 'Clover', 'Digital', 'Stacked']
    readonly property var presetLabels: ['Numeral', 'Scallop', 'Day', 'Clover', 'Digital', 'Stack']
    readonly property var presetIcons: [MD.Token.icon.schedule, MD.Token.icon.filter_vintage, MD.Token.icon.event, MD.Token.icon.local_florist, MD.Token.icon.pin, MD.Token.icon.view_agenda]
    readonly property var formats: [MD.TimeState.LocaleFormat, MD.TimeState.Hour12, MD.TimeState.Hour24]
    readonly property var shapes: [MD.MaterialShape.Cookie12Sided, MD.MaterialShape.Flower, MD.MaterialShape.Clover4Leaf, MD.MaterialShape.Square]
    readonly property var shapeNames: ['Cookie', 'Flower', 'Clover', 'Square']

    MD.ClockSource {
        id: fixedSource
        mode: MD.ClockSource.Fixed
        fixedDateTime: new Date(Date.UTC(2026, 9, 16, 10, 9, 30))
    }
    Column {
        id: content
        width: root.width
        spacing: 20
        Column {
            objectName: 'clockOptions'
            width: parent.width
            spacing: 8
            MD.Label {
                text: root.presetNames[root.presetIndex]
            }
            MD.SegmentedButtonGroup {
                objectName: 'clockPresetChoice'
                size: MD.Enum.XS
                Accessible.name: 'Style'
                Repeater {
                    model: root.presetNames.length
                    delegate: MD.SegmentedButton {
                        required property int index
                        objectName: 'clockPresetChoice' + index
                        width: Math.min(104, Math.max(0, (root.width + root.presetNames.length - 1) / root.presetNames.length))
                        leftPadding: 8
                        rightPadding: 8
                        spacing: 4
                        text: root.width >= 700 ? root.presetLabels[index] : ''
                        icon.name: root.width < 700 ? root.presetIcons[index] : ''
                        checked: root.presetIndex === index
                        onCheckedChangeRequested: value => {
                            if (value)
                                root.presetIndex = index;
                        }
                        Accessible.name: root.presetNames[index]
                        MD.ToolTip.text: root.presetNames[index]
                        MD.ToolTip.visible: mdState.interactions.hovered
                    }
                }
            }
            MD.Label {
                text: 'Shape'
                visible: root.presetIndex === 1
            }
            MD.SegmentedButtonGroup {
                objectName: 'clockShapeChoice'
                size: MD.Enum.XS
                visible: root.presetIndex === 1
                Accessible.name: 'Shape'
                Repeater {
                    model: root.shapes
                    delegate: MD.SegmentedButton {
                        id: shapeButton
                        required property int index
                        required property int modelData
                        objectName: 'clockShapeChoice' + index
                        width: 64
                        checked: root.shapeIndex === index
                        onCheckedChangeRequested: value => {
                            if (value)
                                root.shapeIndex = index;
                        }
                        Accessible.name: root.shapeNames[index]
                        MD.ToolTip.text: root.shapeNames[index]
                        MD.ToolTip.visible: mdState.interactions.hovered
                        contentItem: Item {
                            implicitWidth: 24
                            implicitHeight: 24
                            MD.Shape {
                                anchors.centerIn: parent
                                width: 24
                                height: 24
                                MD.MaterialShapePath {
                                    shape: shapeButton.modelData
                                    size: Qt.size(24, 24)
                                    fillColor: shapeButton.mdState.textColor
                                    strokeWidth: 0
                                }
                            }
                        }
                    }
                }
            }
        }
        MD.SegmentedButtonGroup {
            objectName: 'clockBackgroundChoice'
            size: MD.Enum.XS
            visible: root.presetIndex === 4 || root.presetIndex === 5
            Accessible.name: 'Background'
            MD.SegmentedButton {
                objectName: 'clockSolidChoice'
                width: 128
                text: 'Solid'
                checked: !root.transparentStyle
                onCheckedChangeRequested: value => {
                    if (value)
                        root.transparentStyle = false;
                }
            }
            MD.SegmentedButton {
                objectName: 'clockTransparentChoice'
                width: 144
                text: 'Transparent'
                checked: root.transparentStyle
                onCheckedChangeRequested: value => {
                    if (value)
                        root.transparentStyle = true;
                }
            }
        }
        Flow {
            width: parent.width
            spacing: 12
            MD.SegmentedButtonGroup {
                objectName: 'clockSourceChoice'
                size: MD.Enum.XS
                Accessible.name: 'Time source'
                MD.SegmentedButton {
                    objectName: 'clockLiveChoice'
                    width: 104
                    text: 'Live'
                    checked: !root.fixedTime
                    onCheckedChangeRequested: value => {
                        if (value)
                            root.fixedTime = false;
                    }
                }
                MD.SegmentedButton {
                    objectName: 'clockFixedChoice'
                    width: 104
                    text: 'Fixed'
                    checked: root.fixedTime
                    onCheckedChangeRequested: value => {
                        if (value)
                            root.fixedTime = true;
                    }
                }
            }
            MD.SegmentedButtonGroup {
                objectName: 'clockHourFormatChoice'
                size: MD.Enum.XS
                Accessible.name: 'Hour format'
                MD.SegmentedButton {
                    objectName: 'clockAutoChoice'
                    width: 96
                    text: 'Auto'
                    checked: root.formatIndex === 0
                    onCheckedChangeRequested: value => {
                        if (value)
                            root.formatIndex = 0;
                    }
                }
                MD.SegmentedButton {
                    objectName: 'clock12HourChoice'
                    width: 96
                    text: '12h'
                    checked: root.formatIndex === 1
                    onCheckedChangeRequested: value => {
                        if (value)
                            root.formatIndex = 1;
                    }
                }
                MD.SegmentedButton {
                    objectName: 'clock24HourChoice'
                    width: 96
                    text: '24h'
                    checked: root.formatIndex === 2
                    onCheckedChangeRequested: value => {
                        if (value)
                            root.formatIndex = 2;
                    }
                }
            }
        }
        Flow {
            width: parent.width
            spacing: 16
            Row {
                spacing: 8
                MD.Switch {
                    checked: root.seconds
                    onCheckedChangeRequested: value => root.seconds = value
                    Accessible.name: 'Seconds'
                }
                MD.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: 'Seconds'
                }
            }
            Row {
                spacing: 8
                visible: root.presetIndex === 4 || root.presetIndex === 5
                MD.Switch {
                    checked: root.period
                    onCheckedChangeRequested: value => root.period = value
                    Accessible.name: 'Period'
                }
                MD.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: 'Period'
                }
            }
            Row {
                spacing: 8
                visible: root.presetIndex !== 0 && root.presetIndex !== 3
                MD.Switch {
                    checked: root.date
                    onCheckedChangeRequested: value => root.date = value
                    Accessible.name: 'Date'
                }
                MD.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: 'Date'
                }
            }
            Row {
                spacing: 8
                visible: root.presetIndex === 4 || root.presetIndex === 5
                MD.Switch {
                    checked: root.showAlarm
                    onCheckedChangeRequested: value => root.showAlarm = value
                    Accessible.name: 'Alarm'
                }
                MD.Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: 'Alarm'
                }
            }
        }
        ClockWidget {
            objectName: 'clockPreview'
            x: (parent.width - width) / 2
            width: Math.min(analog ? 320 : stacked ? 220 : 400, parent.width)
            height: analog ? width : stacked ? 320 : 200
            source: root.fixedTime ? fixedSource : null
            style: root.presets[Math.min(root.presetIndex, root.presets.length - 1)]
            solid: !root.transparentStyle
            alarmText: root.showAlarm ? (root.alarmTime?.displayText ?? '') : ''
            hourFormat: root.formats[root.formatIndex]
            showSeconds: root.seconds
            showDate: root.date
            showPeriod: root.period
            shape: root.shapes[root.shapeIndex]
        }
    }
}
