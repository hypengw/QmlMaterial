import QtQuick
import Qcm.Material as MD
import "qrc:/Qcm/Material/Example/compositions" as Demo

Rectangle {
    id: root
    width: 800
    height: 800
    color: MD.MProp.color.surface
    MD.MProp.backgroundColor: color
    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.ClockSource {
        id: clockSource
        mode: MD.ClockSource.Fixed
        fixedDateTime: new Date('2026-10-16T10:09:30Z')
    }
    Column {
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 24
        Demo.MediaCard {
            width: parent.width
            playing: true
            animationsEnabled: false
            expanded: false
        }
        Demo.ClockWidget {
            x: (parent.width - width) / 2
            width: Math.min(280, parent.width)
            height: width
            source: clockSource
            style: Demo.ClockWidget.ScallopStyle
            showDate: true
            showSeconds: true
            locale: Qt.locale('en_US')
        }
    }
}
