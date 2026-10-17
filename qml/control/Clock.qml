pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.ClockBase {
    id: control

    property int shape: MD.MaterialShape.Cookie12Sided
    property bool showTicks: preset === MD.ClockBase.Analog
    property string label: ''
    property color faceColor: MD.MProp.color.secondary_container
    property color handColor: MD.MProp.color.on_secondary_container
    property color secondHandColor: MD.MProp.color.primary
    property color tickColor: MD.MProp.color.on_secondary_container
    property color textColor: MD.MProp.color.on_surface
    property real hourHandWidth: 6
    property real minuteHandWidth: 4
    property real secondHandWidth: 2
    property real hourHandLength: 0.24
    property real minuteHandLength: 0.34
    property real secondHandLength: 0.36
    property Component hourHand: Rectangle {
        radius: width / 2
        color: control.handColor
    }
    property Component minuteHand: Rectangle {
        radius: width / 2
        color: control.handColor
    }
    property Component secondHand: Rectangle {
        radius: width / 2
        color: control.secondHandColor
    }
    readonly property bool analog: preset === MD.ClockBase.Analog || preset === MD.ClockBase.ExpressiveAnalog

    font.pixelSize: 64
    font.weight: Font.Medium
    font.letterSpacing: 0
    spacing: 12
    implicitWidth: (analog ? 240 : preset === MD.ClockBase.StackedDigital ? 180 : Math.max(240, timeMetrics.width)) + leftPadding + rightPadding
    implicitHeight: (analog ? 240 : preset === MD.ClockBase.StackedDigital ? 184 : 88) + (footer.visible ? footer.implicitHeight + spacing : 0) + topPadding + bottomPadding
    Accessible.role: Accessible.StaticText
    Accessible.name: label.length ? label + ', ' + accessibleText : accessibleText

    TextMetrics {
        id: timeMetrics
        font: control.font
        text: control.displayText
    }
    contentItem: Item {
        Item {
            id: face
            width: parent.width
            height: Math.max(0, parent.height - (footer.visible ? footer.height + control.spacing : 0))
            Item {
                id: dial
                anchors.centerIn: parent
                width: Math.max(0, Math.min(parent.width, parent.height))
                height: width
                visible: control.analog
                Rectangle {
                    anchors.fill: parent
                    visible: control.preset === MD.ClockBase.Analog
                    radius: width / 2
                    color: control.faceColor
                }
                MD.Shape {
                    anchors.fill: parent
                    visible: control.preset === MD.ClockBase.ExpressiveAnalog
                    MD.MaterialShapePath {
                        shape: control.shape
                        size: Qt.size(dial.width, dial.height)
                        fillColor: control.faceColor
                        strokeWidth: 0
                    }
                }
                Repeater {
                    model: control.showTicks ? control.ticks : []
                    delegate: Rectangle {
                        required property var modelData
                        width: modelData.major ? 2 : 1
                        height: dial.width * (modelData.major ? 0.04 : 0.015)
                        x: modelData.x * dial.width - width / 2
                        y: modelData.y * dial.height - height / 2
                        rotation: modelData.angle
                        radius: width / 2
                        color: control.tickColor
                        opacity: modelData.major ? 0.8 : 0.4
                    }
                }
                Item {
                    x: dial.width / 2
                    y: dial.height / 2
                    rotation: control.hourAngle
                    Loader {
                        x: -width / 2
                        y: -height
                        width: control.hourHandWidth
                        height: dial.width * control.hourHandLength
                        sourceComponent: control.hourHand
                    }
                }
                Item {
                    x: dial.width / 2
                    y: dial.height / 2
                    rotation: control.minuteAngle
                    Loader {
                        x: -width / 2
                        y: -height
                        width: control.minuteHandWidth
                        height: dial.width * control.minuteHandLength
                        sourceComponent: control.minuteHand
                    }
                }
                Item {
                    x: dial.width / 2
                    y: dial.height / 2
                    visible: control.showSeconds
                    rotation: control.secondAngle
                    Loader {
                        x: -width / 2
                        y: -height
                        width: control.secondHandWidth
                        height: dial.width * control.secondHandLength
                        sourceComponent: control.secondHand
                    }
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.max(control.hourHandWidth, control.minuteHandWidth) + 2
                    height: width
                    radius: width / 2
                    color: control.handColor
                }
            }
            MD.Label {
                anchors.fill: parent
                visible: control.preset === MD.ClockBase.Digital
                text: control.displayText
                font: control.font
                useTypescale: false
                lineHeightMode: Text.ProportionalHeight
                lineHeight: 1
                fontSizeMode: Text.Fit
                minimumPixelSize: 8
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
                color: control.textColor
                Accessible.ignored: true
            }
            Column {
                id: stacked
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                visible: control.preset === MD.ClockBase.StackedDigital
                Repeater {
                    model: [control.hourText, control.minuteText]
                    delegate: MD.Label {
                        required property string modelData
                        width: stacked.width
                        height: Math.max(0, (face.height - detail.height) / 2)
                        text: modelData
                        font: control.font
                        useTypescale: false
                        lineHeightMode: Text.ProportionalHeight
                        lineHeight: 1
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 8
                        horizontalAlignment: Text.AlignHCenter
                        color: control.textColor
                        Accessible.ignored: true
                    }
                }
                MD.Label {
                    id: detail
                    width: stacked.width
                    readonly property string period: control.showPeriod ? control.periodText : ''
                    height: (control.showSeconds || period.length > 0) ? MD.Token.typescale.title_medium.line_height : 0
                    visible: control.showSeconds || period.length > 0
                    text: (control.showSeconds ? control.secondText : '') + (control.showSeconds && period.length ? ' ' : '') + period
                    horizontalAlignment: Text.AlignHCenter
                    typescale: MD.Token.typescale.title_medium
                    color: control.textColor
                    Accessible.ignored: true
                }
            }
        }
        MD.Label {
            id: footer
            y: face.height + control.spacing
            width: parent.width
            visible: control.showDate || control.label.length > 0
            text: (control.showDate ? control.dateText : '') + (control.showDate && control.label.length ? '\n' : '') + control.label
            typescale: MD.Token.typescale.body_medium
            horizontalAlignment: Text.AlignHCenter
            color: control.textColor
            Accessible.ignored: true
        }
    }
}
