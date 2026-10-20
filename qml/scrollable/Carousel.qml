pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.CarouselBase {
    id: control

    property int layout: MD.Enum.CarouselUncontained
    property int orientation: Qt.Horizontal
    // int count, QVariantList, QAbstractItemModel, or JS array (static; use QAbstractItemModel for live updates)
    property var model
    property Component delegate: MD.CarouselImageDelegate {}
    property real itemExtent: MD.Token.carousel.default_item_extent
    property bool itemSnapping: true
    property int initialItem: 0
    property real itemSpacing: MD.Token.carousel.item_spacing
    property real contentPadding: MD.Token.spacing.medium
    property real contentPaddingVertical: MD.Token.carousel.container_padding_vertical
    property real minSmallItemWidth: MD.Token.carousel.small_item_min
    property real maxSmallItemWidth: MD.Token.carousel.small_item_max
    property bool reduceMotion: false
    property bool clipContainer: layout === MD.Enum.CarouselUncontained || layout === MD.Enum.CarouselUncontainedMultiAspect || layout === MD.Enum.CarouselHero || layout === MD.Enum.CarouselHeroCenter ? false : true
    readonly property real contentPaddingEndValue: {
        if (layout === MD.Enum.CarouselUncontained || layout === MD.Enum.CarouselUncontainedMultiAspect || layout === MD.Enum.CarouselFullScreen) {
            return 0;
        }
        return control.contentPadding;
    }
    property bool showPageIndicator: false
    property bool showNavigationButtons: false
    property bool wheelNavigationEnabled: true
    property real minimumViewportHeight: MD.Token.carousel.container_height_horizontal
    property string header: ''

    readonly property bool wheelRequiresShift: control.layout !== MD.Enum.CarouselFullScreen

    readonly property bool hasChromeRow: control.showPageIndicator || control.showNavigationButtons
    readonly property real chromeOverhead: control.hasChromeRow ? control.chromeRowHeight + control.chromeRowSpacing : 0
    readonly property real chromeRowHeight: MD.Token.carousel.chrome_row_height
    readonly property real chromeRowSpacing: MD.Token.carousel.chrome_row_spacing
    readonly property real tokenViewportHeight: control.layout === MD.Enum.CarouselFullScreen ? MD.Token.carousel.default_item_extent : control.minimumViewportHeight
    readonly property real viewportHeight: control.orientation === Qt.Vertical ? Math.max(0, control.height - control.chromeOverhead) : Math.max(control.tokenViewportHeight, Math.max(0, control.height - control.chromeOverhead))

    view: m_view
    previousControl: m_previous
    nextControl: m_next

    implicitWidth: orientation === Qt.Horizontal ? MD.Token.carousel.default_width : MD.Token.carousel.default_height_vertical
    implicitHeight: orientation === Qt.Vertical ? MD.Token.carousel.default_height_vertical : (tokenViewportHeight + chromeOverhead)

    focusPolicy: Qt.NoFocus

    Accessible.role: Accessible.Grouping
    Accessible.name: control.header.length > 0 ? control.header : 'Carousel'

    Binding on reduceMotion {
        when: typeof Qt.styleHints !== 'undefined' && typeof Qt.styleHints.prefersReducedMotion === 'boolean'
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
            mirrored: control.mirrored
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

            wheelNavigationEnabled: control.wheelNavigationEnabled
            wheelRequiresShift: control.wheelRequiresShift
        }

        RowLayout {
            layoutDirection: control.layoutDirection
            visible: control.hasChromeRow
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: control.chromeRowSpacing

            MD.StandardIconButton {
                id: m_previous
                visible: control.showNavigationButtons && (control.view?.count ?? 0) > 1
                icon.name: control.orientation === Qt.Horizontal ? (control.mirrored ? MD.Token.icon.chevron_right : MD.Token.icon.chevron_left) : MD.Token.icon.expand_less
                enabled: control.currentIndex > 0
            }

            MD.PageIndicator {
                visible: control.showPageIndicator && (control.view?.count ?? 0) > 1
                Layout.fillWidth: control.showNavigationButtons
                count: control.view?.count ?? 0
                currentIndex: control.currentIndex
            }

            MD.StandardIconButton {
                id: m_next
                visible: control.showNavigationButtons && (control.view?.count ?? 0) > 1
                icon.name: control.orientation === Qt.Horizontal ? (control.mirrored ? MD.Token.icon.chevron_left : MD.Token.icon.chevron_right) : MD.Token.icon.expand_more
                enabled: control.currentIndex < (control.view?.count ?? 0) - 1
            }
        }
    }
}
