import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

Item {
    id: root
    width: 720
    height: 1600

    readonly property var demoModel: [
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-coral-landscape.jpg',
            title: 'Coral'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-mint-square.jpg',
            title: 'Mint'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-violet-portrait.jpg',
            title: 'Violet'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-sun-ultrawide.jpg',
            title: 'Sun'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-teal-tall.jpg',
            title: 'Teal'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-amber-wide.jpg',
            title: 'Amber'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-rose-medium.jpg',
            title: 'Rose'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-sky-small.jpg',
            title: 'Sky'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-lime-diagonal.jpg',
            title: 'Lime'
        },
        {
            imageUrl: 'qrc:/Qcm/Material/Example/assets/carousel/abstract-indigo-portrait.jpg',
            title: 'Indigo'
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 20

        CarouselScene {
            title: 'Multi-browse (rest)'
            layout: MD.Enum.CarouselMultiBrowse
            scrollOffset: 0
        }

        CarouselScene {
            title: 'Multi-browse (mid-scroll)'
            layout: MD.Enum.CarouselMultiBrowse
            scrollOffset: 120
        }

        CarouselScene {
            title: 'Multi-browse (end of list)'
            layout: MD.Enum.CarouselMultiBrowse
            initialItem: 9
            showPageIndicator: true
        }

        CarouselScene {
            title: 'Uncontained (rest)'
            layout: MD.Enum.CarouselUncontained
            itemExtent: 140
            scrollOffset: 0
        }

        CarouselScene {
            title: 'Uncontained (mid-scroll)'
            layout: MD.Enum.CarouselUncontained
            itemExtent: 140
            scrollOffset: 100
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
            scrollOffset: 0
        }

        CarouselScene {
            title: 'Uncontained multi-aspect (mid-scroll)'
            layout: MD.Enum.CarouselUncontainedMultiAspect
            scrollOffset: 120
        }

        CarouselScene {
            title: 'Uncontained multi-aspect (end of list)'
            layout: MD.Enum.CarouselUncontainedMultiAspect
            initialItem: 5
        }

        CarouselScene {
            title: 'Hero (rest)'
            layout: MD.Enum.CarouselHero
            scrollOffset: 0
        }

        CarouselScene {
            title: 'Hero (mid-scroll)'
            layout: MD.Enum.CarouselHero
            initialItem: 4
        }

        CarouselScene {
            title: 'Hero center (rest)'
            layout: MD.Enum.CarouselHeroCenter
            scrollOffset: 0
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
            scrollOffset: -1
        }
    }

    component CarouselScene: ColumnLayout {
        id: scene
        Layout.fillWidth: true
        property string title
        property int layout
        property real itemExtent: MD.Token.carousel.default_item_extent
        property int initialItem: 0
        property real scrollOffset: 0
        property bool showPageIndicator: false

        spacing: 4

        MD.Text {
            text: scene.title
            typescale: MD.Token.typescale.label_medium
            color: MD.MProp.color.on_surface
        }

        MD.Carousel {
            id: carousel
            Layout.fillWidth: true
            layout: scene.layout
            itemExtent: scene.itemExtent
            model: root.demoModel
            delegate: MD.CarouselImageDelegate {}
            initialItem: scene.initialItem
            showPageIndicator: scene.showPageIndicator

            Component.onCompleted: {
                if (scene.scrollOffset >= 0) {
                    carousel.debugScrollOffset = scene.scrollOffset;
                }
            }
        }
    }
}
