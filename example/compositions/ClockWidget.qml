pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ClockBase {
    id: root
    enum Style {
        NumeralStyle,
        ScallopStyle,
        DayStyle,
        CloverStyle,
        DigitalStyle,
        StackedStyle
    }
    property int style: ClockWidget.NumeralStyle
    property bool solid: true
    property int shape: MD.MaterialShape.Cookie12Sided
    property string alarmText: ''
    property color faceColor: MD.MProp.color.primary_container
    property color handColor: MD.MProp.color.secondary
    property color minuteHandColor: MD.MProp.color.primary
    property color secondHandColor: MD.MProp.color.tertiary
    property color numeralColor: textColor
    property color textColor: MD.MProp.color.on_primary_container
    readonly property bool analog: style <= ClockWidget.CloverStyle
    readonly property bool stacked: style === ClockWidget.StackedStyle
    implicitWidth: analog ? 280 : stacked ? 184 : 320
    implicitHeight: analog ? 280 : stacked ? 272 : 160
    preset: analog ? (style === ClockWidget.ScallopStyle || style === ClockWidget.CloverStyle ? MD.ClockBase.ExpressiveAnalog : MD.ClockBase.Analog) : stacked ? MD.ClockBase.StackedDigital : MD.ClockBase.Digital
    Accessible.role: Accessible.StaticText
    Accessible.name: accessibleText + (alarmText.length ? ', ' + alarmText : '')

    FontLoader {
        id: mediumFont
        objectName: 'widgetMediumFont'
        source: '../assets/clocks/GoogleSansFlex-Medium.ttf'
    }
    contentItem: Item {
        Item {
            id: artwork
            objectName: 'widgetArtwork'
            width: root.analog ? 240 : root.stacked ? 184 : 320
            height: root.analog ? 240 : root.stacked ? 272 : 160
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            scale: Math.max(0, Math.min(parent.width / width, parent.height / height))

            Rectangle {
                objectName: 'widgetSolidBackground'
                anchors.fill: parent
                visible: root.analog ? root.style === ClockWidget.NumeralStyle || root.style === ClockWidget.DayStyle : root.solid
                radius: Math.min(width, height) / 2
                color: root.faceColor
            }
            MD.Shape {
                anchors.fill: parent
                visible: root.style === ClockWidget.ScallopStyle || root.style === ClockWidget.CloverStyle
                MD.MaterialShapePath {
                    shape: root.style === ClockWidget.CloverStyle ? MD.MaterialShape.Cookie4Sided : root.shape
                    size: Qt.size(artwork.width, artwork.height)
                    fillColor: root.faceColor
                    strokeWidth: 0
                }
            }
            Item {
                anchors.fill: parent
                visible: root.analog
                ClockWidgetNumerals {
                    objectName: 'widgetNumerals'
                    anchors.fill: parent
                    visible: root.style === ClockWidget.NumeralStyle
                    color: root.numeralColor
                }
                Rectangle {
                    objectName: 'widgetDayWindow'
                    x: 180
                    y: 101
                    width: 44
                    height: 38
                    radius: 10
                    color: root.faceColor
                    visible: root.style === ClockWidget.DayStyle && root.showDate
                    MD.Label {
                        anchors.fill: parent
                        font.family: mediumFont.name
                        font.pixelSize: 22
                        useTypescale: false
                        horizontalAlignment: Text.AlignHCenter
                        color: root.textColor
                        text: root.dayText
                        Accessible.ignored: true
                    }
                }
                Item {
                    objectName: 'widgetOrbitDate'
                    x: 120
                    y: 120
                    rotation: root.secondAngle + 180
                    visible: root.style === ClockWidget.ScallopStyle && root.showDate
                    MD.Label {
                        x: -36
                        y: -102
                        width: 72
                        height: 24
                        font.family: mediumFont.name
                        font.pixelSize: 14
                        useTypescale: false
                        color: root.textColor
                        horizontalAlignment: Text.AlignHCenter
                        text: root.weekdayText + ' ' + root.dayText
                        Accessible.ignored: true
                    }
                }
                ClockWidgetHand {
                    objectName: 'widgetHourHand'
                    anchors.fill: parent
                    kind: root.style === ClockWidget.ScallopStyle ? ClockWidgetHand.ScallopHour : root.style === ClockWidget.DayStyle ? ClockWidgetHand.DayHour : ClockWidgetHand.SimpleHour
                    rotation: root.hourAngle
                    color: root.handColor
                }
                ClockWidgetHand {
                    objectName: 'widgetMinuteHand'
                    anchors.fill: parent
                    kind: root.style === ClockWidget.ScallopStyle ? ClockWidgetHand.ScallopMinute : ClockWidgetHand.SimpleMinute
                    rotation: root.minuteAngle
                    color: root.minuteHandColor
                }
                ClockWidgetHand {
                    objectName: 'widgetSecondHand'
                    anchors.fill: parent
                    kind: root.style === ClockWidget.ScallopStyle ? ClockWidgetHand.ScallopSecond : ClockWidgetHand.SimpleSecond
                    rotation: root.secondAngle
                    visible: root.showSeconds
                    color: root.secondHandColor
                }
            }
            Item {
                anchors.fill: parent
                visible: !root.analog
                MD.Label {
                    objectName: 'widgetDigitalDate'
                    x: 12
                    y: root.stacked ? 24 : 20
                    width: parent.width - 24
                    height: 24
                    visible: root.showDate
                    font.family: mediumFont.name
                    font.pixelSize: 14
                    useTypescale: false
                    horizontalAlignment: Text.AlignHCenter
                    color: root.textColor
                    text: root.compactDateText
                    Accessible.ignored: true
                }
                MD.Label {
                    objectName: 'widgetDigitalTime'
                    x: 16
                    y: root.showDate ? 48 : 34
                    width: parent.width - 32
                    height: 72
                    visible: !root.stacked
                    font.family: mediumFont.name
                    font.pixelSize: 64
                    useTypescale: false
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 24
                    lineHeightMode: Text.ProportionalHeight
                    lineHeight: 1
                    wrapMode: Text.NoWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: root.textColor
                    text: root.displayText
                    Accessible.ignored: true
                }
                Column {
                    x: 16
                    y: 54
                    width: parent.width - 32
                    visible: root.stacked
                    Repeater {
                        model: [root.hourText, root.minuteText]
                        delegate: MD.Label {
                            required property string modelData
                            width: parent.width
                            height: 72
                            font.family: mediumFont.name
                            font.pixelSize: 64
                            useTypescale: false
                            lineHeightMode: Text.ProportionalHeight
                            lineHeight: 1
                            color: root.textColor
                            text: modelData
                            horizontalAlignment: Text.AlignHCenter
                            Accessible.ignored: true
                        }
                    }
                }
                MD.Label {
                    x: 12
                    y: 202
                    width: parent.width - 24
                    height: 24
                    visible: root.stacked
                    font.family: mediumFont.name
                    font.pixelSize: 16
                    useTypescale: false
                    horizontalAlignment: Text.AlignHCenter
                    color: root.textColor
                    text: (root.showSeconds ? root.secondText : '') + (root.showSeconds && root.showPeriod && root.periodText.length ? ' ' : '') + (root.showPeriod ? root.periodText : '')
                    Accessible.ignored: true
                }
                Row {
                    objectName: 'widgetAlarm'
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: root.stacked ? 238 : 124
                    spacing: 4
                    visible: root.alarmText.length > 0
                    MD.Icon {
                        objectName: 'widgetAlarmIcon'
                        name: MD.Token.icon.alarm
                        size: 16
                        color: root.textColor
                    }
                    MD.Label {
                        text: root.alarmText
                        font.family: mediumFont.name
                        font.pixelSize: 14
                        useTypescale: false
                        color: root.textColor
                    }
                }
            }
        }
    }
}
