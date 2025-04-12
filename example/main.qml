import QtQuick
import QtQuick.Window
// this is needed for qml wasm to load QtQuick.Shape plugin
import QtQuick.Controls.Basic
import Qcm.Material as MD

MD.ApplicationWindow {
    height: 600
    visible: true
    width: 900

    MD.MProp.textColor: MD.MProp.color.on_surface
    MD.MProp.backgroundColor: MD.MProp.color.surface_container

    Loader {
        anchors.fill: parent
        source: 'qrc:/Qcm/Material/Example/Example.qml'
    }
}
