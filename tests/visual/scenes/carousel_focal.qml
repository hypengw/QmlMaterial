import QtQuick
import Qcm.Material as MD

Rectangle {
    id: root
    width: 1024
    height: 1830
    color: "white"
    readonly property var pictures: ["abstract-coral-landscape.jpg", "abstract-mint-square.jpg", "abstract-sky-small.jpg", "abstract-violet-portrait.jpg", "abstract-sun-ultrawide.jpg", "abstract-rose-medium.jpg", "abstract-teal-tall.jpg", "abstract-amber-wide.jpg", "abstract-lime-diagonal.jpg", "abstract-indigo-portrait.jpg"]
    readonly property var imageModel: pictures.map((name, index) => ({
                imageUrl: "qrc:/Qcm/Material/Example/assets/carousel/" + name,
                title: String(index)
            }))
    readonly property var rows: [
        {
            layout: MD.Enum.CarouselMultiBrowse,
            label: "Multi-browse",
            index: 0
        },
        {
            layout: MD.Enum.CarouselMultiBrowse,
            label: "Multi-browse",
            index: 4
        },
        {
            layout: MD.Enum.CarouselMultiBrowse,
            label: "Multi-browse",
            index: 9
        },
        {
            layout: MD.Enum.CarouselHero,
            label: "Hero",
            index: 0
        },
        {
            layout: MD.Enum.CarouselHero,
            label: "Hero",
            index: 4
        },
        {
            layout: MD.Enum.CarouselHero,
            label: "Hero",
            index: 9
        },
        {
            layout: MD.Enum.CarouselHeroCenter,
            label: "Hero center",
            index: 0
        },
        {
            layout: MD.Enum.CarouselHeroCenter,
            label: "Hero center",
            index: 4
        },
        {
            layout: MD.Enum.CarouselHeroCenter,
            label: "Hero center",
            index: 9
        }
    ]
    Repeater {
        model: 18
        Item {
            id: row
            required property int index
            readonly property var sample: root.rows[Math.floor(index / 2)]
            x: (index % 2) * 512
            y: Math.floor(index / 2) * 200
            width: 512
            height: 200
            MD.Label {
                x: 16
                y: 8
                text: row.sample.label + " / " + row.sample.index + (row.index % 2 ? " / RTL" : " / LTR")
                color: "black"
            }
            MD.Carousel {
                x: 0
                y: 40
                width: 512
                height: 144
                minimumViewportHeight: 144
                layoutDirection: row.index % 2 ? Qt.RightToLeft : Qt.LeftToRight
                layout: row.sample.layout
                initialItem: row.sample.index
                model: root.imageModel
            }
        }
    }
}
