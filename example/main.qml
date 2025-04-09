import QtQuick
import QtQuick.Window
// this is needed for qml wasm to load QtQuick.Shape plugin
import QtQuick.Controls.Basic
import Qcm.Material as MD

MD.ApplicationWindow {
    height: 600
    visible: true
    width: 900

    MD.MatProp.textColor: MD.MatProp.color.on_surface
    MD.MatProp.backgroundColor: MD.MatProp.color.surface_container

    Loader {
        anchors.fill: parent
        source: 'qrc:/Qcm/Material/Example/Example.qml'
    }
}
