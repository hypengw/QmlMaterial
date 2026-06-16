pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Control {
    id: control

    property int layout: MD.Enum.CarouselUncontained
    property int alignment: MD.Enum.CarouselAlignStart
    property int orientation: Qt.Horizontal
    property var model
    property Component delegate: MD.CarouselImageDelegate {}
    property real itemExtent: MD.Token.carousel.default_item_extent
    property var flexWeights: null
    property bool itemSnapping: true
    property real shrinkExtent: 0
    property bool infinite: false
    property alias currentIndex: m_view.currentIndex
    property int initialItem: 0
    property real itemSpacing: MD.Token.carousel.item_spacing
    property real contentPadding: MD.Token.spacing.medium
    property real contentPaddingVertical: MD.Token.carousel.container_padding_vertical
    property real preferredItemWidth: itemExtent
    property real minSmallItemWidth: MD.Token.carousel.small_item_min
    property real maxSmallItemWidth: MD.Token.carousel.small_item_max
    property bool reduceMotion: false
    property bool clipContainer: layout === MD.Enum.CarouselUncontained
                                 || layout === MD.Enum.CarouselUncontainedMultiAspect ? false : true
    readonly property real contentPaddingEndValue: {
        if (layout === MD.Enum.CarouselUncontained
                || layout === MD.Enum.CarouselUncontainedMultiAspect
                || layout === MD.Enum.CarouselFullScreen) {
            return 0;
        }
        return control.contentPadding;
    }
    property alias debugScrollOffset: m_view.debugScrollOffset
    property bool showPageIndicator: false
    property bool showNavigationButtons: false
    property MD.StateCarousel mdState: MD.StateCarousel {
        item: control
    }

    signal clicked(int index)
    signal indexChanged(int index)

    function incrementCurrentIndex() {
        m_view.incrementCurrentIndex();
    }

    function decrementCurrentIndex() {
        m_view.decrementCurrentIndex();
    }

    function setCurrentIndex(index, animated) {
        if (animated) {
            m_view.setCurrentIndexAnimated(index);
        } else {
            m_view.currentIndex = index;
        }
    }

    implicitWidth: orientation === Qt.Horizontal ? 480 : 320
    implicitHeight: orientation === Qt.Vertical
        ? 320
        : (layout === MD.Enum.CarouselFullScreen
           ? MD.Token.carousel.default_item_extent
           : MD.Token.carousel.container_height_horizontal)

    focusPolicy: Qt.StrongFocus

    Keys.onLeftPressed: event => {
        if (orientation === Qt.Horizontal) {
            decrementCurrentIndex();
            event.accepted = true;
        }
    }
    Keys.onRightPressed: event => {
        if (orientation === Qt.Horizontal) {
            incrementCurrentIndex();
            event.accepted = true;
        }
    }
    Keys.onUpPressed: event => {
        if (orientation === Qt.Vertical) {
            decrementCurrentIndex();
            event.accepted = true;
        }
    }
    Keys.onDownPressed: event => {
        if (orientation === Qt.Vertical) {
            incrementCurrentIndex();
            event.accepted = true;
        }
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Home) {
            setCurrentIndex(0, true);
            event.accepted = true;
        } else if (event.key === Qt.Key_End && m_view.count > 0) {
            setCurrentIndex(m_view.count - 1, true);
            event.accepted = true;
        }
    }

    contentItem: ColumnLayout {
        spacing: 8

        CarouselView {
            id: m_view
            Layout.fillWidth: true
            Layout.fillHeight: !control.showPageIndicator && !control.showNavigationButtons
            Layout.preferredHeight: control.orientation === Qt.Vertical
                ? control.height - (control.showPageIndicator ? 32 : 0)
                : (control.layout === MD.Enum.CarouselFullScreen
                   ? MD.Token.carousel.default_item_extent
                   : MD.Token.carousel.container_height_horizontal)

            model: control.model
            delegate: control.delegate
            layout: control.layout
            alignment: control.alignment
            orientation: control.orientation
            itemExtent: control.preferredItemWidth
            minSmallItemWidth: control.minSmallItemWidth
            maxSmallItemWidth: control.maxSmallItemWidth
            flexWeights: control.flexWeights
            itemSnapping: control.itemSnapping
            shrinkExtent: control.shrinkExtent
            infinite: control.infinite
            initialItem: control.initialItem
            spacing: control.itemSpacing
            contentPaddingStart: control.layout === MD.Enum.CarouselFullScreen ? 0 : control.contentPadding
            contentPaddingEnd: control.contentPaddingEndValue
            contentPaddingCross: control.layout === MD.Enum.CarouselFullScreen ? 0 : control.contentPaddingVertical
            clipContainer: control.clipContainer
            reduceMotion: control.reduceMotion
            itemCornerRadius: MD.Token.carousel.item_corner
            interactive: control.enabled

            onClicked: index => control.clicked(index)
            onCurrentIndexChanged: control.indexChanged(m_view.currentIndex)
        }

        RowLayout {
            visible: control.showPageIndicator || control.showNavigationButtons
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            MD.StandardIconButton {
                visible: control.showNavigationButtons && m_view.count > 1
                icon.name: control.orientation === Qt.Horizontal
                    ? MD.Token.icon.chevron_left
                    : MD.Token.icon.expand_less
                enabled: m_view.currentIndex > 0
                onClicked: control.decrementCurrentIndex()
            }

            MD.PageIndicator {
                visible: control.showPageIndicator && m_view.count > 1
                Layout.fillWidth: control.showNavigationButtons
                count: m_view.count
                currentIndex: m_view.currentIndex
            }

            MD.StandardIconButton {
                visible: control.showNavigationButtons && m_view.count > 1
                icon.name: control.orientation === Qt.Horizontal
                    ? MD.Token.icon.chevron_right
                    : MD.Token.icon.expand_more
                enabled: m_view.currentIndex < m_view.count - 1
                onClicked: control.incrementCurrentIndex()
            }
        }
    }

    MD.WheelHandler {
        target: m_view.flickable
        scrollFlickableTarget: true
    }
}
