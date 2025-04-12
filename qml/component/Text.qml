import QtQuick
import QtQuick.Window
import Qcm.Material as MD

Text {
    id: root
    property MD.t_typescale typescale: MD.Token.typescale.label_medium
    property bool prominent: false
    property bool useTypescale: true

    Binding {
        when: root.useTypescale
        root.lineHeight: root.typescale ? root.typescale?.line_height : 16
        root.font.pixelSize: (root.typescale ? root.typescale?.size : 16)
        root.font.weight: root.typescale ? (root.prominent ? root.typescale.weight_prominent : typescale.weight) : Font.Normal
        root.font.letterSpacing: root.typescale ? root.typescale.tracking : 1
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
