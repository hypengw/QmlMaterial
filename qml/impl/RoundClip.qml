import QtQuick
import Qcm.Material as MD

ShaderEffect {
    property var source
    property var radius: 0
    property vector4d radius_: MD.Util.corner(radius).toVector4D()
    property vector2d size: Qt.vector2d(1,1)
    property real smoothing: 1.0
    property real offset: -1.0

    vertexShader: 'qrc:/Qcm/Material/assets/shader/default.vert.qsb'
    fragmentShader: 'qrc:/Qcm/Material/assets/shader/round_clip.frag.qsb'
}
