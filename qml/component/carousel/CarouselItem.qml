pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
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

    readonly property bool down: m_area.pressed
    readonly property bool hovered: m_area.containsMouse
    property MD.StateCarouselItem mdState: MD.StateCarouselItem {
        item: root
    }

    readonly property real visibleWidth: Math.max(0, width * (1 - maskStart - maskEnd))
    readonly property real visibleHeight: Math.max(0, height * (1 - maskStart - maskEnd))
    // M3 carousel peek: pill when width = 2 * corner radius (cap radius to half visible size).
    readonly property real effectiveCornerRadius: {
        const maxRadius = MD.Token.carousel.item_corner;
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

    property QtObject _carouselView: null

    focus: visible && index === carouselCurrentIndex
    activeFocusOnTab: focus

    Accessible.role: Accessible.Button
    Accessible.name: {
        const pos = root.index + 1;
        const total = root.carouselCount > 0 ? root.carouselCount : 1;
        const prefix = root.accessibilityTitle.length > 0 ? root.accessibilityTitle + ', ' : '';
        return prefix + pos + ' of ' + total;
    }

    visible: (1 - maskStart - maskEnd) > 0.001

    Keys.onPressed: event => {
        const horizontal = root.carouselOrientation === Qt.Horizontal;
        const prevKey = horizontal ? Qt.Key_Left : Qt.Key_Up;
        const nextKey = horizontal ? Qt.Key_Right : Qt.Key_Down;

        if (event.key === prevKey) {
            if (root._carouselView) {
                root._carouselView.decrementCurrentIndexFromKeyboard();
            }
            event.accepted = true;
        } else if (event.key === nextKey) {
            if (root._carouselView) {
                root._carouselView.incrementCurrentIndexFromKeyboard();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Space || event.key === Qt.Key_Return) {
            root.clicked();
            event.accepted = true;
        }
    }

    onActiveFocusChanged: {
        if (!root._carouselView) {
            return;
        }
        if (activeFocus && !m_area.pressed) {
            root._carouselView.engageTabFocus();
        } else if (!activeFocus && root.index === root.carouselCurrentIndex) {
            root._carouselView.suppressFocusRing();
        }
    }

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
            pressX: m_area.mouseX
            pressY: m_area.mouseY
            pressed: m_area.pressed
            stateOpacity: root.mdState.stateLayerOpacity
            color: root.mdState.stateLayerColor
        }

        MouseArea {
            id: m_area
            anchors.fill: parent
            hoverEnabled: true
            onPressed: {
                if (root._carouselView) {
                    root._carouselView.claimInteractionFocus();
                }
            }
            onClicked: root.clicked()
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

    signal clicked()
}
