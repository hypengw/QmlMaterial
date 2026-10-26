import QtQuick
import QtQuick.Effects
import QtQuick.Window

import Qcm.Material as MD

Item {
    id: root

    property bool colorOverlay: false
    property color color: MD.MProp.color.on_background
    property int size: 24
    property string sourceData
    property alias source: image.source
    property alias sourceSize: image.sourceSize

    implicitWidth: size
    implicitHeight: size

    Image {
        id: image
        anchors.fill: parent
        // Qt does not apply SVG DPR scaling to data URLs.
        sourceSize.width: Math.ceil(width * (source.toString().startsWith("data:") ? Screen.devicePixelRatio : 1))
        sourceSize.height: Math.ceil(height * (source.toString().startsWith("data:") ? Screen.devicePixelRatio : 1))
        mipmap: true
        smooth: root.smooth
        cache: true
        source: 'data:image/svg+xml;utf8,' + root.sourceData
    }

    layer.enabled: colorOverlay
    layer.effect: MultiEffect {
        brightness: 1.0
        colorization: 1.0
        colorizationColor: root.color
    }
}
