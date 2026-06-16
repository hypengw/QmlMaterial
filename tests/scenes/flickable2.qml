import QtQuick
import Qcm.Material as MD

Rectangle {
    width: 480
    height: 360
    color: MD.Token.color.surface

    MD.Flickable2 {
        id: view
        anchors.centerIn: parent
        width: 320
        height: 220
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
        inputMaskMode: MD.Flickable2.ContentOnly

        Column {
            id: contentColumn
            width: view.width
            spacing: 8

            Repeater {
                model: 10

                Rectangle {
                    required property int index
                    width: view.width
                    height: 44
                    radius: 8
                    color: index % 2 ? MD.Token.color.primary_container : MD.Token.color.secondary_container

                    MD.Label {
                        anchors.centerIn: parent
                        text: "Flickable2 row " + (index + 1)
                    }
                }
            }
        }
    }
}
