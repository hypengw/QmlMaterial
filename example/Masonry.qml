pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Page {
    padding: 0
    title: 'Masonry'

    MD.LazyMasonryGrid {
        id: view

        anchors.fill: parent
        bottomMargin: 16
        crossSpacing: 12
        estimatedItemExtent: 180
        leftMargin: 16
        mainSpacing: 12
        minimumCellExtent: 160
        rightMargin: 16
        topMargin: 16

        MD.ScrollBarBase.vertical: MD.ScrollBar {}
        delegate: Item {
            required property MD.LazyRow row

            implicitHeight: width * row.value.ratio

            Image {
                anchors.fill: parent
                asynchronous: true
                fillMode: Image.PreserveAspectCrop
                source: 'qrc:/Qcm/Material/Example/assets/carousel/' + parent.row.value.image
            }
        }
        source: MD.ListSnapshotSource {
            items: [
                {
                    id: 'amber',
                    image: 'abstract-amber-wide.jpg',
                    ratio: 0.56
                },
                {
                    id: 'indigo',
                    image: 'abstract-indigo-portrait.jpg',
                    ratio: 1.5
                },
                {
                    id: 'mint',
                    image: 'abstract-mint-square.jpg',
                    ratio: 1
                },
                {
                    id: 'coral',
                    image: 'abstract-coral-landscape.jpg',
                    ratio: 0.67
                },
                {
                    id: 'violet',
                    image: 'abstract-violet-portrait.jpg',
                    ratio: 1.5
                },
                {
                    id: 'teal',
                    image: 'abstract-teal-tall.jpg',
                    ratio: 2
                },
                {
                    id: 'sun',
                    image: 'abstract-sun-ultrawide.jpg',
                    ratio: 0.4
                },
                {
                    id: 'rose',
                    image: 'abstract-rose-medium.jpg',
                    ratio: 0.8
                }
            ]
            keyRole: 'id'
        }
    }
}
