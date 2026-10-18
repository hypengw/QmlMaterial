import QtQuick
import Qcm.Material as MD

MD.Dialog {
    id: root
    objectName: 'typographyPopup'
    title: 'Typography'
    standardButtons: MD.Dialog.Close
    width: Math.min(640, Math.max(0, overlayWidth - 32))
    height: Math.min(implicitHeight, Math.max(0, overlayHeight - 32))
    positioningItem: overlayItem
    x: (overlayWidth - width) / 2
    y: (overlayHeight - height) / 2

    contentItem: MD.VerticalFlickable {
        objectName: 'typographySamples'
        implicitWidth: 592
        implicitHeight: Math.min(480, content.implicitHeight + topMargin + bottomMargin)
        topMargin: 8
        bottomMargin: 8
        clip: true

        Column {
            id: content
            width: parent.width
            spacing: 16

            Repeater {
                model: ['display_large', 'display_medium', 'display_small', 'headline_large', 'headline_medium', 'headline_small', 'title_large', 'title_medium', 'title_small', 'label_large', 'label_medium', 'label_small', 'body_large', 'body_medium', 'body_small',]
                MD.Text {
                    required property string modelData
                    width: content.width
                    typescale: MD.Token.typescale[modelData]
                    text: modelData.replace('_', ' ')
                    font.capitalization: Font.Capitalize
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
