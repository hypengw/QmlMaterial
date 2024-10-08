import QtQuick
import QtQuick.Controls
import QtQuick.Window

ApplicationWindow {
    height: 600
    visible: true
    width: 900

    Loader {
        source: 'qrc:/Qcm/Material/Example/Example.qml'
    } 
}
