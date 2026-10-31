pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import Qcm.Material as MD

Item {
    id: root
    property MD.BackdropSurface surface
    readonly property real padding: surface?.capturePadding ?? 0
    ShaderEffectSource {
        id: capture
        width: root.width + 2 * root.padding
        height: root.height + 2 * root.padding
        visible: false
        sourceItem: root.surface?.sourceItem ?? null
        sourceRect: root.surface?.captureRect ?? Qt.rect(0, 0, 0, 0)
        textureSize: root.surface?.captureSize ?? Qt.size(1, 1)
        hideSource: false
        recursive: false
        live: true
    }
    Item {
        id: mask
        width: capture.width
        height: capture.height
        visible: false
        layer.enabled: true
        layer.textureSize: capture.textureSize
        Rectangle {
            x: root.padding
            y: root.padding
            width: root.width
            height: root.height
            radius: root.surface?.radius ?? 0
            color: "white"
            antialiasing: true
        }
    }
    MultiEffect {
        x: -root.padding
        y: -root.padding
        width: capture.width
        height: capture.height
        source: capture
        autoPaddingEnabled: false
        blurEnabled: true
        blurMax: 32
        blur: root.surface?.blur ?? 0
        maskEnabled: true
        maskSource: mask
    }
}
