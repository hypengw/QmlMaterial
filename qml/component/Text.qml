import QtQuick
import QtQuick.Window
import Qcm.Material as MD

Text {
    id: root
    property MD.typescale typescale: MD.Token.typescale.label_medium
    property bool prominent: false
    property bool useTypescale: true

    Binding {
        when: root.useTypescale
        root.lineHeight: root.typescale.line_height
        root.font.pixelSize: root.typescale.size
        root.font.weight: root.prominent ? root.typescale.weight_prominent : root.typescale.weight
        root.font.letterSpacing: root.typescale.tracking
        restoreMode: Binding.RestoreNone
    }

    antialiasing: true

    color: MD.MProp.textColor
    lineHeightMode: Text.FixedHeight
    wrapMode: Text.Wrap
    elide: Text.ElideRight
    maximumLineCount: 1
    textFormat: Text.PlainText
}
