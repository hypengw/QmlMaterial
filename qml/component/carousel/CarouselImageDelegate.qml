pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.CarouselItem {
    id: root
    required property var model
    property string imageUrl: ''
    property string url: ''
    property string source: ''
    property string title: ''

    signal aspectRatioChanged()

    readonly property real itemAspectRatio: {
        if (m_image.status === Image.Ready && m_image.sourceSize.height > 0) {
            return m_image.sourceSize.width / m_image.sourceSize.height;
        }
        if (model !== undefined && model !== null && typeof model !== 'string' && model.aspectRatio !== undefined) {
            return model.aspectRatio;
        }
        return 1.0;
    }

    onItemAspectRatioChanged: root.aspectRatioChanged()

    showBackground: false

    readonly property string imageSource: {
        if (root.imageUrl.length > 0) {
            return root.imageUrl;
        }
        if (root.url.length > 0) {
            return root.url;
        }
        if (root.source.length > 0) {
            return root.source;
        }
        if (model === undefined || model === null) {
            return '';
        }
        if (typeof model === 'string') {
            return model;
        }
        if (model.imageUrl !== undefined && model.imageUrl !== '') {
            return model.imageUrl;
        }
        if (model.url !== undefined && model.url !== '') {
            return model.url;
        }
        if (model.source !== undefined && model.source !== '') {
            return model.source;
        }
        return '';
    }

    readonly property string itemTitle: {
        if (root.title.length > 0) {
            return root.title;
        }
        if (model === undefined || model === null || typeof model === 'string') {
            return '';
        }
        return model.title ?? '';
    }

    // M3 Carousel.md: title alpha fades over 80dp of mask inset (onMaskChangedListener).
    readonly property real titleOpacity: {
        const fadeSpan = 80;
        const leftMasked = root.maskStart * root.width;
        const rightMasked = root.maskEnd * root.width;
        const leftFade = leftMasked > 0 ? Math.max(0, 1 - leftMasked / fadeSpan) : 1;
        const rightFade = rightMasked > 0 ? Math.max(0, 1 - rightMasked / fadeSpan) : 1;
        return Math.min(leftFade, rightFade);
    }

    Item {
        id: m_media
        anchors.fill: parent
        clip: true
        x: -root.parallaxShift

        MD.Image {
            id: m_image
            anchors.fill: parent
            source: root.imageSource
            fillMode: Image.PreserveAspectCrop
            radius: MD.Token.shape.corner.extra_large
            elevation: MD.Token.elevation.level0
            asynchronous: true
        }

        MD.Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: MD.Token.spacing.medium
            visible: root.itemTitle.length > 0
            opacity: root.titleOpacity
            text: root.itemTitle
            typescale: MD.Token.typescale.title_small
            // M3 carousel / Flutter Colors.white: label on photo is always light; background is the image, not app surface.
            color: Qt.rgba(1, 1, 1, 1)
            verticalAlignment: Text.AlignBottom
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }
    }
}
