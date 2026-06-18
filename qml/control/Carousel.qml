pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

T.Control {
    id: control

    property int layout: MD.Enum.CarouselUncontained
    property int orientation: Qt.Horizontal
    // int count, QVariantList, QAbstractItemModel, or JS array (static; use QAbstractItemModel for live updates)
    property var model
    property Component delegate: MD.CarouselImageDelegate {}
    property real itemExtent: MD.Token.carousel.default_item_extent
    property bool itemSnapping: true
    property alias currentIndex: m_view.currentIndex
    property int initialItem: 0
    property real itemSpacing: MD.Token.carousel.item_spacing
    property real contentPadding: MD.Token.spacing.medium
    property real contentPaddingVertical: MD.Token.carousel.container_padding_vertical
    property real minSmallItemWidth: MD.Token.carousel.small_item_min
    property real maxSmallItemWidth: MD.Token.carousel.small_item_max
    property bool reduceMotion: false
    property bool clipContainer: layout === MD.Enum.CarouselUncontained
                                 || layout === MD.Enum.CarouselUncontainedMultiAspect
                                 || layout === MD.Enum.CarouselHero
                                 || layout === MD.Enum.CarouselHeroCenter ? false : true
    readonly property real contentPaddingEndValue: {
        if (layout === MD.Enum.CarouselUncontained
                || layout === MD.Enum.CarouselUncontainedMultiAspect
                || layout === MD.Enum.CarouselFullScreen) {
            return 0;
        }
        return control.contentPadding;
    }
    property bool showPageIndicator: false
    property bool showNavigationButtons: false
    property string header: ''

    readonly property bool hasChromeRow: control.showPageIndicator || control.showNavigationButtons
    readonly property real chromeOverhead: control.hasChromeRow
        ? control.chromeRowHeight + control.chromeRowSpacing
        : 0
    readonly property real chromeRowHeight: MD.Token.carousel.chrome_row_height
    readonly property real chromeRowSpacing: MD.Token.carousel.chrome_row_spacing
    readonly property real tokenViewportHeight: control.layout === MD.Enum.CarouselFullScreen
        ? MD.Token.carousel.default_item_extent
        : MD.Token.carousel.container_height_horizontal
    readonly property real viewportHeight: control.orientation === Qt.Vertical
        ? Math.max(0, control.height - control.chromeOverhead)
        : Math.max(control.tokenViewportHeight,
                   Math.max(0, control.height - control.chromeOverhead))

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

    implicitWidth: orientation === Qt.Horizontal
        ? MD.Token.carousel.default_width
        : MD.Token.carousel.default_height_vertical
    implicitHeight: orientation === Qt.Vertical
        ? MD.Token.carousel.default_height_vertical
        : (viewportHeight + chromeOverhead)

    focusPolicy: Qt.NoFocus

    Accessible.role: Accessible.Grouping
    Accessible.name: control.header.length > 0 ? control.header : 'Carousel'

    Binding on reduceMotion {
        when: typeof Qt.styleHints !== 'undefined'
              && typeof Qt.styleHints.prefersReducedMotion === 'boolean'
        value: Qt.styleHints.prefersReducedMotion
    }

    contentItem: ColumnLayout {
        spacing: control.chromeRowSpacing

        CarouselView {
            id: m_view
            Layout.fillWidth: true
            Layout.fillHeight: control.orientation === Qt.Vertical
            Layout.preferredHeight: control.viewportHeight

            model: control.model
            delegate: control.delegate
            layout: control.layout
            orientation: control.orientation
            itemExtent: control.itemExtent
            minSmallItemWidth: control.minSmallItemWidth
            maxSmallItemWidth: control.maxSmallItemWidth
            itemSnapping: control.itemSnapping
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
            visible: control.hasChromeRow
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: control.chromeRowSpacing

            MD.StandardIconButton {
                visible: control.showNavigationButtons && m_view.count > 1
                icon.name: control.orientation === Qt.Horizontal
                    ? MD.Token.icon.chevron_left
                    : MD.Token.icon.expand_less
                enabled: m_view.currentIndex > 0
                onClicked: {
                    m_view.claimInteractionFocus();
                    control.decrementCurrentIndex();
                }
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
                onClicked: {
                    m_view.claimInteractionFocus();
                    control.incrementCurrentIndex();
                }
            }
        }
    }

    MD.WheelHandler {
        target: m_view.flickable
        scrollFlickableTarget: true
        pageScrollModifiers: Qt.ShiftModifier
        onWheelMoved: m_view.claimInteractionFocus()
    }
}
