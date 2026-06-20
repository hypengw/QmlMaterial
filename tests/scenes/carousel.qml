import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import "../../example/carousel" as CarouselDemo

Item {
    id: root
    width: 720
    height: 1600

    CarouselDemo.CarouselDemoData {
        id: m_carousel_demo
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 20

        CarouselScene {
            title: 'Multi-browse (rest)'
            layout: MD.Enum.CarouselMultiBrowse
        }

        CarouselScene {
            title: 'Multi-browse (mid-scroll)'
            layout: MD.Enum.CarouselMultiBrowse
            initialItem: 2
        }

        CarouselScene {
            title: 'Multi-browse (end of list)'
            layout: MD.Enum.CarouselMultiBrowse
            initialItem: 9
            showPageIndicator: true
        }

        CarouselScene {
            title: 'Multi-browse (nav buttons)'
            layout: MD.Enum.CarouselMultiBrowse
            initialItem: 2
            showNavigationButtons: true
        }

        CarouselScene {
            layout: MD.Enum.CarouselUncontained
            itemExtent: 140
        }

        CarouselScene {
            title: 'Uncontained (mid-scroll)'
            layout: MD.Enum.CarouselUncontained
            itemExtent: 140
            initialItem: 2
        }

        CarouselScene {
            title: 'Uncontained (end of list)'
            layout: MD.Enum.CarouselUncontained
            itemExtent: 180
            initialItem: 5
        }

        CarouselScene {
            title: 'Uncontained multi-aspect (rest)'
            layout: MD.Enum.CarouselUncontainedMultiAspect
        }

        CarouselScene {
            title: 'Uncontained multi-aspect (mid-scroll)'
            layout: MD.Enum.CarouselUncontainedMultiAspect
            initialItem: 2
        }

        CarouselScene {
            title: 'Uncontained multi-aspect (end of list)'
            layout: MD.Enum.CarouselUncontainedMultiAspect
            initialItem: 5
        }

        CarouselScene {
            title: 'Hero (rest)'
            layout: MD.Enum.CarouselHero
        }

        CarouselScene {
            title: 'Hero (mid-scroll)'
            layout: MD.Enum.CarouselHero
            initialItem: 4
        }

        CarouselScene {
            title: 'Hero center (rest)'
            layout: MD.Enum.CarouselHeroCenter
        }

        CarouselScene {
            title: 'Hero center (mid-scroll)'
            layout: MD.Enum.CarouselHeroCenter
            initialItem: 4
        }

        CarouselScene {
            title: 'Hero center (end of list)'
            layout: MD.Enum.CarouselHeroCenter
            initialItem: 9
        }

        CarouselScene {
            title: 'Full-screen (last item)'
            layout: MD.Enum.CarouselFullScreen
            initialItem: 3
        }

        CarouselScene {
            title: 'Full-screen (vertical)'
            layout: MD.Enum.CarouselFullScreen
            orientation: Qt.Vertical
            preferredHeight: 360
            showPageIndicator: true
        }
    }

    component CarouselScene: ColumnLayout {
        id: scene
        Layout.fillWidth: true
        property string title
        property int layout
        property real itemExtent: MD.Token.carousel.default_item_extent
        property int initialItem: 0
        property bool showPageIndicator: false
        property bool showNavigationButtons: false
        property int orientation: Qt.Horizontal
        property real preferredHeight: -1

        spacing: 4

        MD.Text {
            text: scene.title
            typescale: MD.Token.typescale.label_medium
            color: MD.MProp.color.on_surface
        }

        MD.Carousel {
            id: carousel
            Layout.fillWidth: true
            Layout.preferredHeight: scene.preferredHeight > 0 ? scene.preferredHeight : implicitHeight
            layout: scene.layout
            orientation: scene.orientation
            itemExtent: scene.itemExtent
            model: m_carousel_demo.model
            delegate: MD.CarouselImageDelegate {}
            initialItem: scene.initialItem
            showPageIndicator: scene.showPageIndicator
            showNavigationButtons: scene.showNavigationButtons
        }
    }
}
