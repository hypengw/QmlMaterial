import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Window

import Qcm.Material as MD

ApplicationWindow {
    height: 600
    visible: true
    width: 900

    MD.MatProp.textColor: MD.MatProp.color.on_surface
    MD.MatProp.backgroundColor: MD.MatProp.color.surface
    Loader {
        anchors.fill: parent
        source: 'qrc:/Qcm/Material/Example/Example.qml'
    } 
}
