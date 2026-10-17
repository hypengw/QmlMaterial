pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 900
    height: 760
    color: MD.MProp.color.surface
    MD.ClockSource {
        id: source
        mode: MD.ClockSource.Fixed
        fixedDateTime: new Date(Date.UTC(2026, 9, 16, 10, 9, 30))
    }
    Grid {
        anchors.fill: parent
        anchors.margins: 24
        columns: root.width < 600 ? 1 : 2
        spacing: 24
        Repeater {
            model: [MD.ClockBase.Analog, MD.ClockBase.ExpressiveAnalog, MD.ClockBase.Digital, MD.ClockBase.StackedDigital]
            delegate: MD.Clock {
                required property int modelData
                width: (root.width - 48 - (parent.columns - 1) * parent.spacing) / parent.columns
                height: 332
                source: source
                preset: modelData
                timeZoneId: 'UTC'
                locale: Qt.locale('en_US')
                hourFormat: MD.TimeState.Hour12
                showSeconds: true
                showDate: true
                label: ['Analog', 'Expressive analog', 'Digital', 'Stacked digital'][modelData]
            }
        }
    }
}
