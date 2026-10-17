import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 900
    height: 900
    color: MD.MProp.color.surface
    MD.MProp.backgroundColor: color
    MD.MProp.textColor: MD.MProp.color.on_surface
    Loader {
        anchors.fill: parent
        source: 'qrc:/Qcm/Material/Example/Compositions.qml'
    }
}
