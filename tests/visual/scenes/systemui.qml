import QtQuick
import Qcm.Material as MD
import "qrc:/Qcm/Material/Example/compositions" as Demo

Rectangle {
    id: root
    width: 900
    height: 660
    color: MD.MProp.color.surface
    MD.MProp.backgroundColor: color
    MD.MProp.textColor: MD.MProp.color.on_surface
    Column {
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 24
        Demo.QuickControls {
            width: parent.width
            columns: width >= 680 ? 3 : 2
            rows: columns === 3 ? 3 : 4
        }
        Demo.NotificationList {
            width: parent.width
            animationsEnabled: false
        }
    }
}
