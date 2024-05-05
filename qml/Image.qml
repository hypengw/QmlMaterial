import QtQuick
import Qcm.Material as MD

Item {
    id: root
    // tl,tr,bl,br
    property var radius: [0]
    property alias asynchronous: m_image.asynchronous
    property alias autoTransform: m_image.autoTransform
    property alias cache: m_image.cache
    property alias currentFrame: m_image.currentFrame
    property alias fillMode: m_image.fillMode
    property alias frameCount: m_image.frameCount
    property alias horizontalAlignment: m_image.horizontalAlignment
    property alias mipmap: m_image.mipmap
    property alias mirror: m_image.mirror
    property alias mirrorVertically: m_image.mirrorVertically
    property alias progress: m_image.progress
    property alias source: m_image.source
    property alias sourceClipRect: m_image.sourceClipRect
    property alias sourceSize: m_image.sourceSize
    property alias status: m_image.status
    property alias verticalAlignment: m_image.verticalAlignment

    property alias paintedHeight: m_image.paintedHeight
    property alias paintedWidth: m_image.paintedWidth

    implicitHeight: {
        return sourceSize.height;
    }
    implicitWidth: {
        return sourceSize.width;
    }
    opacity: root.status === Image.Ready && root.paintedHeight > 0 ? 1 : 0

    MD.MatProp.elevation: MD.Token.elevation.level0

    Image {
        id: m_image
        anchors.fill: parent
        cache: true
        smooth: true
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        layer.effect: MD.RoundClip {
            radius: root.radius
            size: root.height
            layer.enabled: true
            layer.effect: MD.RoundedElevationEffect {
                elevation: root.opacity > 0 ? root.MD.MatProp.elevation : MD.Token.elevation.level0
            }
        }
    }
}
