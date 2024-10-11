import QtQuick
import Qcm.Material as MD

Item {
    id: root
    property var radius: 0
    property alias color: inner.color

    ShaderEffect {
        id: inner
        anchors.centerIn: parent
        width: root.width
        height: root.height

        property vector4d radius: MD.Util.corner(root.radius).toVector4D()
        property vector4d size: Qt.vector4d(width, height, root.width - 1, root.height - 1) // -1 to make edge keep same as qt's
        property color color: "#00000000"
        property real smoothing: 1.0
        property real offset: 0.0
        vertexShader: 'qrc:/Qcm/Material/assets/shader/default.vert.qsb'
        fragmentShader: 'qrc:/Qcm/Material/assets/shader/round_rect.frag.qsb'
    }
}
