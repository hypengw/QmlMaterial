import QtQuick
import Qcm.Material as MD

/** @ingroup component */
Item {
    id: root
    required property MD.IconSpec icon

    implicitWidth: icon.empty ? 0 : Math.max(0, icon.width)
    implicitHeight: icon.empty ? 0 : Math.max(0, icon.height)
    readonly property int status: icon.image ? (m_loader.item?.status ?? Image.Null) : Image.Null

    Loader {
        id: m_loader
        anchors.fill: parent
        active: !root.icon.empty
        sourceComponent: root.icon.image ? imageIcon : fontIcon
    }

    Component {
        id: fontIcon
        MD.Icon {
            name: root.icon.name
            size: Math.max(0, Math.min(root.width, root.height, root.icon.width, root.icon.height))
            color: root.icon.color
            fill: root.icon.fill
            weight: root.icon.weight
            renderType: root.icon.renderType
        }
    }

    Component {
        id: imageIcon
        Image {
            source: root.icon.resolvedSource
            sourceSize.width: Math.max(0, root.icon.width)
            sourceSize.height: Math.max(0, root.icon.height)
            fillMode: Image.PreserveAspectFit
            cache: root.icon.cache
            asynchronous: true
            // Transparent color preserves the original image colors, like Qt icons.
            layer.enabled: root.icon.color.a > 0
            layer.effect: ShaderEffect {
                property var source
                property color tint: root.icon.color
                vertexShader: "qrc:/Qcm/Material/assets/shader/default.vert.qsb"
                fragmentShader: "qrc:/Qcm/Material/assets/shader/icon_tint.frag.qsb"
            }
        }
    }
}
