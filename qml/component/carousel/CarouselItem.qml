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
    readonly property bool down: m_area.pressed
    readonly property bool hovered: m_area.containsMouse
    property MD.StateCarouselItem mdState: MD.StateCarouselItem {
        item: root
    }

    readonly property real visibleWidth: Math.max(0, width * (1 - maskStart - maskEnd))
    readonly property real visibleHeight: Math.max(0, height * (1 - maskStart - maskEnd))

    property bool showBackground: true

    default property alias content: m_content.data

    implicitWidth: CarouselView.itemWidth
    implicitHeight: CarouselView.itemHeight

    clip: true
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
            corners: MD.Util.corners(MD.Token.shape.corner.extra_large)
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

        MD.FocusIndicator {
            corners: m_bg.corners
            active: root.activeFocus
        }

        MouseArea {
            id: m_area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.clicked()
        }
    }

    signal clicked()
}
