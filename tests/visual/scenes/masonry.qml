pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Rectangle {
    color: MD.Token.color.surface
    height: 400
    width: 480

    MD.LazyMasonryGrid {
        anchors.fill: parent
        bottomMargin: 12
        crossSpacing: 8
        estimatedItemExtent: 100
        fixedSlotCount: 3
        leftMargin: 12
        mainSpacing: 8
        rightMargin: 12
        topMargin: 12

        delegate: Rectangle {
            required property MD.LazyRow row

            color: row.index % 2 ? MD.Token.color.secondary_container : MD.Token.color.primary_container
            implicitHeight: row.value.h
            radius: 8

            MD.Text {
                anchors.centerIn: parent
                color: MD.Token.color.on_surface
                text: parent.row.key
            }
        }
        source: MD.ListSnapshotSource {
            items: [
                {
                    id: 'a',
                    h: 180
                },
                {
                    id: 'b',
                    h: 80
                },
                {
                    id: 'c',
                    h: 120
                },
                {
                    id: 'd',
                    h: 160
                },
                {
                    id: 'e',
                    h: 96
                },
                {
                    id: 'f',
                    h: 72
                },
                {
                    id: 'g',
                    h: 180
                },
                {
                    id: 'h',
                    h: 80
                },
                {
                    id: 'i',
                    h: 120
                }
            ]
            keyRole: 'id'
        }
    }
}
