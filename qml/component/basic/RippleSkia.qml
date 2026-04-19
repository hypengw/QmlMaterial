pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    property real stateOpacity: 0
    property bool pressed: false
    property real pressX: width / 2
    property real pressY: height / 2

    property alias color: m_back.color
    property alias corners: m_back.corners
    property alias radius: m_back.radius

    property real in_progress: 0
    property real in_noisePhase: 0
    property real in_turbulencePhase: 0

    implicitWidth: 100
    implicitHeight: 100
    clip: false

    MD.Rectangle {
        id: m_back
        anchors.fill: parent
        color: "transparent"
        opacity: root.stateOpacity
    }

    ShaderEffect {
        id: shader
        anchors.fill: parent
        opacity: 0

        property vector2d in_origin: Qt.vector2d(root.width / 2, root.height / 2)
        property vector2d in_touch: Qt.vector2d(root.pressX, root.pressY)
        property real in_progress: root.in_progress
        property real in_maxRadius: Math.sqrt(root.width * root.width + root.height * root.height) * 1.1
        property vector2d in_resolutionScale: Qt.vector2d(1.0 / root.width, 1.0 / root.height)
        property vector2d in_noiseScale: Qt.vector2d(1.0 / 2.0, 1.0 / 2.0)
        property real in_hasMask: 1.0
        property real in_noisePhase: root.in_noisePhase
        property real in_turbulencePhase: root.in_turbulencePhase
        
        property vector2d in_tCircle1: Qt.vector2d(0.5, 0.5)
        property vector2d in_tCircle2: Qt.vector2d(0.2, 0.2)
        property vector2d in_tCircle3: Qt.vector2d(0.5, 0.8)
        property vector2d in_tRotation1: Qt.vector2d(1.0, 0.0)
        property vector2d in_tRotation2: Qt.vector2d(0.9659, 0.2588)
        property vector2d in_tRotation3: Qt.vector2d(0.8660, 0.5)
        
        property color in_color: root.color
        property color in_sparkleColor: Qt.rgba(1, 1, 1, 0.5)
        property vector2d in_size: Qt.vector2d(root.width, root.height)
        property vector4d in_corners: root.corners.toVector4D()

        vertexShader: "qrc:/Qcm/Material/assets/shader/default.vert.qsb"
        fragmentShader: "qrc:/Qcm/Material/assets/shader/ripple.frag.qsb"
    }

    NumberAnimation {
        id: noiseAnim
        target: root
        property: "in_noisePhase"
        from: 0
        to: 1000
        duration: 100000
        loops: Animation.Infinite
        running: root.in_progress > 0 && root.in_progress < 1.0
    }

    NumberAnimation {
        id: turbAnim
        target: root
        property: "in_turbulencePhase"
        from: 0
        to: 1000
        duration: 200000
        loops: Animation.Infinite
        running: root.in_progress > 0 && root.in_progress < 1.0
    }

    SequentialAnimation {
        id: pressAnim
        NumberAnimation {
            target: root
            property: "in_progress"
            to: 0.4
            duration: 450
            easing.type: Easing.OutCubic
        }
    }

    SequentialAnimation {
        id: releaseAnim
        NumberAnimation {
            target: root
            property: "in_progress"
            to: 1.0
            duration: 350
            easing.type: Easing.OutCubic
        }
        PropertyAction {
            target: root
            property: "in_progress"
            value: 0
        }
    }

    onPressedChanged: {
        if (pressed) {
            releaseAnim.stop()
            m_fade.stop()
            m_back.opacity = 0.12
            shader.opacity = 0.12
            pressAnim.start()
        } else {
            pressAnim.stop()
            releaseAnim.start()
            m_fade.start()
        }
    }

    SequentialAnimation {
        id: m_fade
        running: false
        onStopped: {
            m_back.opacity = Qt.binding(function () {
                return root.stateOpacity;
            });
            shader.opacity = 0;
        }

        NumberAnimation {
            target: m_back
            to: root.stateOpacity
            duration: 150
            property: "opacity"
        }
        NumberAnimation {
            target: shader
            to: 0
            duration: 150
            property: "opacity"
        }
    }
}
