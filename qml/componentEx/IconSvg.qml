import QtQuick
import QtQuick.Effects

import Qcm.Material as MD

Image {
    id: root

    property bool colorOverlay: false
    property color color: MD.MatProp.color.on_background
    property int size: 24

    sourceSize.width: size
    sourceSize.height: size
    mipmap: true
    smooth: true
    cache: true

    layer.enabled: colorOverlay
    layer.effect: MultiEffect {
        brightness: 1.0
        colorization: 1.0
        colorizationColor: root.color
    }
}