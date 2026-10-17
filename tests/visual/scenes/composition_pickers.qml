import QtQuick
import Qcm.Material as MD
import "qrc:/Qcm/Material/Example/compositions" as Demo

Rectangle {
    width: 880
    height: 1450
    color: MD.MProp.color.surface
    MD.MProp.backgroundColor: color
    MD.MProp.textColor: MD.MProp.color.on_surface
    Flow {
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 24
        Demo.Clocks {
            width: Math.min(400, parent.width)
            fixedTime: true
        }
        Demo.TimePickers {
            width: Math.min(400, parent.width)
        }
        Demo.DatePickers {
            width: Math.min(400, parent.width)
        }
        Column {
            width: Math.min(400, parent.width)
            spacing: 24
            Demo.ColorPickers {
                width: parent.width
            }
            Demo.FilePickers {
                width: parent.width
            }
        }
    }
}
