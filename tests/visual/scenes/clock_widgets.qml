pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import "qrc:/Qcm/Material/Example/compositions" as Demo

Rectangle {
    id: root
    width: 980
    height: 660
    color: MD.MProp.color.surface
    MD.MProp.backgroundColor: color
    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.ClockSource {
        id: fixedSource
        mode: MD.ClockSource.Fixed
        fixedDateTime: new Date('2026-10-16T10:09:30Z')
    }
    Flow {
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 16
        Repeater {
            model: 6
            delegate: Demo.ClockWidget {
                required property int index
                width: Math.min(300, root.width - 32)
                height: 300
                style: index
                source: fixedSource
                locale: Qt.locale('en_US')
                hourFormat: MD.TimeState.Hour24
                showDate: true
                showSeconds: index < 4
                alarmText: 'Mon 09:30'
            }
        }
    }
}
