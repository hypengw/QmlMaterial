pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

// Material You "realistic" ripple, ported from Skia's RippleShader.rts
// (third_party/skia/resources/sksl/realistic/RippleShader.rts) and the
// driver in third_party/skia/gm/rippleshadergm.cpp.
Item {
    id: root

    // ---- Public Ripple API (mirrors Ripple2.qml) ----
    property real stateOpacity: 0

    property color color: "transparent"
    property MD.InteractionSource interactionSource: null
    property alias indication: motion.indication
    property real pressOpacity: MD.Token.state.pressed.state_layer_opacity
    property bool animationsEnabled: true
    property alias corners: m_back.corners
    property alias radius: m_back.radius

    // ---- Debug interface ----
    // Set debugEnabled = true to drive the shader from fixed values
    // instead of the press/release animation. Useful for visual tests
    // and for poking at the effect interactively.
    property bool debugEnabled: false
    property real debugProgress: 0.5      // 0..1 ripple expansion
    property real debugPhase: 5.0         // noise / turbulence phase
    property real debugTouchX: -1         // <0 uses the center
    property real debugTouchY: -1
    property real debugOpacity: 0.12      // shader visibility while debug
    property real debugBackOpacity: 0.12  // state-layer tint while debug
    // (set < 0 to leave m_back at stateOpacity)

    implicitWidth: 100
    implicitHeight: 100
    clip: false

    // ---- Constants ported from rippleshadergm.cpp ----
    readonly property real _scale: 1.5
    readonly property real _piRotateRight: Math.PI * 0.0078125
    readonly property real _piRotateLeft: Math.PI * -0.0078125

    // ---- Internal animated state ----
    readonly property real _progress: motion.progress
    readonly property real _phase: motion.phase
    readonly property real _runtimeOpacity: motion.rippleOpacity

    readonly property real _shaderOpacity: debugEnabled ? debugOpacity : _runtimeOpacity
    readonly property real _activeProgress: debugEnabled ? debugProgress : _progress
    readonly property real _activePhase: debugEnabled ? debugPhase : _phase
    readonly property real _activeTouchX: debugEnabled ? (debugTouchX >= 0 ? debugTouchX : width / 2) : motion.origin.x
    readonly property real _activeTouchY: debugEnabled ? (debugTouchY >= 0 ? debugTouchY : height / 2) : motion.origin.y
    readonly property real _maxCorner: Math.min(root.width, root.height) * 0.5
    readonly property vector4d _shaderCorners: Qt.vector4d(root._clampCorner(root.corners.bottomRight), root._clampCorner(root.corners.topRight), root._clampCorner(root.corners.bottomLeft), root._clampCorner(root.corners.topLeft))

    function _clampCorner(value) {
        return Math.min(Math.max(value, 0), root._maxCorner);
    }

    // Distance from the touch point to the furthest corner — this is the
    // radius the wave needs to reach so it just covers the whole shape at
    // progress=1, regardless of where the user clicked.
    readonly property real _maxRadius: {
        const tx = _activeTouchX, ty = _activeTouchY;
        const w = Math.max(width, 1), h = Math.max(height, 1);
        return Math.max(Math.hypot(tx, ty), Math.hypot(w - tx, ty), Math.hypot(tx, h - ty), Math.hypot(w - tx, h - ty));
    }

    MD.RippleMotion {
        id: motion
        target: root
        interactionSource: root.interactionSource
        stateOpacity: root.stateOpacity
        pressOpacity: root.pressOpacity
        color: root.color
        enabled: root.animationsEnabled && !root.debugEnabled
    }

    ShaderEffect {
        id: shader
        anchors.fill: parent
        opacity: root._shaderOpacity
        visible: opacity > 0

        readonly property real _ph: root._activePhase

        property vector2d in_origin: Qt.vector2d(root.width / 2, root.height / 2)
        property vector2d in_touch: Qt.vector2d(root._activeTouchX, root._activeTouchY)
        property real in_progress: root._activeProgress
        property real in_maxRadius: root._maxRadius
        // resolutionScale: pixel-to-uv (uv ∈ [0,1])
        property vector2d in_resolutionScale: Qt.vector2d(1.0 / Math.max(root.width, 1), 1.0 / Math.max(root.height, 1))
        // noiseScale: ~2.1px sparkle grid in uv space (matches rippleshadergm.cpp)
        property vector2d in_noiseScale: Qt.vector2d(2.1 / Math.max(root.width, 1), 2.1 / Math.max(root.height, 1))
        property real in_hasMask: 1.0
        property real in_noisePhase: _ph
        property real in_turbulencePhase: _ph * 1000.0

        property vector2d in_tCircle1: Qt.vector2d(root._scale * 0.5 + (_ph * 0.01 * Math.cos(root._scale * 0.55)), root._scale * 0.5 + (_ph * 0.01 * Math.sin(root._scale * 0.55)))
        property vector2d in_tCircle2: Qt.vector2d(root._scale * 0.2 + (_ph * -0.0066 * Math.cos(root._scale * 0.45)), root._scale * 0.2 + (_ph * -0.0066 * Math.sin(root._scale * 0.45)))
        property vector2d in_tCircle3: Qt.vector2d(root._scale + (_ph * -0.0066 * Math.cos(root._scale * 0.35)), root._scale + (_ph * -0.0066 * Math.sin(root._scale * 0.35)))
        property vector2d in_tRotation1: Qt.vector2d(Math.cos(_ph * root._piRotateRight + 1.7 * Math.PI), Math.sin(_ph * root._piRotateRight + 1.7 * Math.PI))
        property vector2d in_tRotation2: Qt.vector2d(Math.cos(_ph * root._piRotateLeft + 2.0 * Math.PI), Math.sin(_ph * root._piRotateLeft + 2.0 * Math.PI))
        property vector2d in_tRotation3: Qt.vector2d(Math.cos(_ph * root._piRotateRight + 2.75 * Math.PI), Math.sin(_ph * root._piRotateRight + 2.75 * Math.PI))

        property color in_color: root.debugEnabled ? root.color : motion.ink
        property color in_sparkleColor: Qt.rgba(1, 1, 1, 0.5)
        property vector2d in_size: Qt.vector2d(root.width, root.height)
        property vector4d in_corners: root._shaderCorners

        vertexShader: "qrc:/Qcm/Material/assets/shader/default.vert.qsb"
        fragmentShader: "qrc:/Qcm/Material/assets/shader/ripple.frag.qsb"
    }

    MD.Rectangle {
        id: m_back
        anchors.fill: parent
        color: root.debugEnabled ? root.color : motion.ink
        opacity: root.debugEnabled ? (root.debugBackOpacity >= 0 ? root.debugBackOpacity : root.stateOpacity) : motion.opacity
    }
}
