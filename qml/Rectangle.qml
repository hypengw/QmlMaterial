import QtQuick
import Qcm.Material as MD

Item {
    id: root
    property var radius: 0
    property alias color: inner.color

    ShaderEffect {
        id: inner
        anchors.centerIn: parent
        width: root.width + 16
        height: root.height + 16

        property vector4d radius: MD.Util.corner(root.radius).toVector4D()
        property vector4d size: Qt.vector4d(width, height, root.width, root.height)
        property color color: "#00000000"
        property real smoothing: 1.0
        property real offset: 0.0
        fragmentShader: 'qrc:/Qcm/Material/assets/shader/round_rect.frag.qsb'
    }
}
