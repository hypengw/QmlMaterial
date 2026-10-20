pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.CarouselItemBase {
    id: root
    required property int index

    property real maskStart: CarouselView.maskStart
    property real maskEnd: CarouselView.maskEnd
    property real parallaxShift: CarouselView.parallaxShift
    property int sizeClass: CarouselView.sizeClass
    property bool isActive: CarouselView.isActive
    readonly property int carouselCount: CarouselView.carouselCount
    readonly property int carouselOrientation: CarouselView.carouselOrientation
    readonly property int carouselCurrentIndex: CarouselView.carouselCurrentIndex
    property string accessibilityTitle: ''
    property real cornerRadius: MD.Token.carousel.item_corner

    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    containmentMask: m_visible
    property MD.StateCarouselItem mdState: MD.StateCarouselItem {
        focusIndicationEnabled: false
        item: root
    }

    readonly property real visibleWidth: Math.max(0, width * (1 - maskStart - maskEnd))
    readonly property real visibleHeight: Math.max(0, height * (1 - maskStart - maskEnd))
    // M3 carousel peek: pill when width = 2 * corner radius (cap radius to half visible size).
    readonly property real effectiveCornerRadius: {
        const maxRadius = root.cornerRadius;
        if (root.visibleWidth <= 0 || root.visibleHeight <= 0) {
            return maxRadius;
        }
        return Math.min(maxRadius, root.visibleWidth / 2, root.visibleHeight / 2);
    }
    readonly property MD.corners effectiveCorners: MD.Util.corners(root.effectiveCornerRadius)

    property bool showBackground: true

    default property alias content: m_content.data

    implicitWidth: CarouselView.itemWidth
    implicitHeight: CarouselView.itemHeight

    // Qt rejects removing the tab stop while this item still owns active focus.
    activeFocusOnTab: activeFocus || (visible && index === carouselCurrentIndex)

    Accessible.role: Accessible.Button
    Accessible.name: {
        const pos = root.index + 1;
        const total = root.carouselCount > 0 ? root.carouselCount : 1;
        const prefix = root.accessibilityTitle.length > 0 ? root.accessibilityTitle + ', ' : '';
        return prefix + pos + ' of ' + total;
    }

    visible: (1 - maskStart - maskEnd) > 0.001

    Item {
        id: m_visible
        x: root.width * root.maskStart
        width: root.visibleWidth
        height: root.height
        clip: true

        MD.ElevationRectangle {
            id: m_bg
            anchors.fill: parent
            visible: root.showBackground
            corners: root.effectiveCorners
            color: root.mdState.backgroundColor
            elevation: root.mdState.elevation
            elevationVisible: root.showBackground
            opacity: root.mdState.backgroundOpacity
        }

        Item {
            id: m_content
            anchors.fill: parent
        }

        MD.Ripple {
            anchors.fill: parent
            corners: m_bg.corners
            interactionSource: root.interactionSource
            indication: root.mdState.indication
            color: root.mdState.stateLayerColor
        }
    }

    MD.FocusIndicator {
        x: root.width * root.maskStart
        width: root.visibleWidth
        height: root.height
        z: 20
        inset: CarouselView.focusRingInset
        active: CarouselView.focusRingVisible
        corners: root.effectiveCorners
    }
}
