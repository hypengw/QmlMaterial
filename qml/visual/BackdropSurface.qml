pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

/** @ingroup component */
MD.BackdropSurfaceBase {
    id: root
    property real radius: 24
    property real blur: 0.8
    property color tint: Qt.rgba(root.MD.MProp.color.surface_container.r, root.MD.MProp.color.surface_container.g, root.MD.MProp.color.surface_container.b, 0.72)
    property color fallbackColor: root.MD.MProp.color.surface_container
    readonly property bool supported: GraphicsInfo.api !== GraphicsInfo.Software && GraphicsInfo.api !== GraphicsInfo.Unknown
    readonly property bool effectAvailable: captureEnabled && supported && effect.status === Loader.Ready

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.fallbackColor
    }
    Loader {
        id: effect
        anchors.fill: parent
        active: root.captureEnabled && root.supported
        source: Qt.resolvedUrl("BackdropEffect.qml")
        onLoaded: item.surface = root
    }
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.tint
        visible: root.effectAvailable
    }
}
