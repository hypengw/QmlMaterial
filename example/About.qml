pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Dialog {
    id: root
    objectName: 'aboutPopup'
    title: 'About'
    standardButtons: MD.Dialog.Close
    width: Math.min(440, Math.max(0, overlayWidth - 32))
    height: Math.min(implicitHeight, Math.max(0, overlayHeight - 32))
    positioningItem: overlayItem
    x: (overlayWidth - width) / 2
    y: (overlayHeight - height) / 2

    contentItem: MD.VerticalFlickable {
        implicitWidth: 392
        implicitHeight: content.implicitHeight
        clip: true
        Column {
            id: content
            width: parent.width
            spacing: 16
            MD.Text {
                width: parent.width
                text: 'QmlMaterial'
                typescale: MD.Token.typescale.title_large
                horizontalAlignment: Text.AlignHCenter
            }
            MD.Text {
                width: parent.width
                text: MD.Token.version
                typescale: MD.Token.typescale.label_large
                horizontalAlignment: Text.AlignHCenter
                opacity: 0.8
            }
            Flow {
                width: parent.width
                spacing: 8
                MD.SuggestionChip {
                    text: 'repo'
                    onClicked: Qt.openUrlExternally('https://github.com/hypengw/QmlMaterial')
                }
                MD.SuggestionChip {
                    text: 'issue'
                    onClicked: Qt.openUrlExternally('https://github.com/hypengw/QmlMaterial/issues')
                }
                MD.SuggestionChip {
                    text: 'donate'
                    onClicked: Qt.openUrlExternally('https://ko-fi.com/hypengw')
                }
            }
        }
    }
}
