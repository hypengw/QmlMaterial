import QtQuick
import Qcm.Material as MD

TextEdit {
    id: root
    property MD.t_typescale typescale: MD.Token.typescale.label_medium
    property bool prominent: false

    Binding {
        when: typescale
        root.font.pixelSize: typescale ? typescale?.size : 16
        root.font.weight: typescale ? (root.prominent ? typescale.weight_prominent : typescale.weight) : Font.Normal
        root.font.letterSpacing: typescale ? typescale.tracking : 1
        restoreMode: Binding.RestoreNone
    }

    cursorDelegate: MD.CursorDelegate {}

    color: MD.MatProp.textColor
    selectionColor: MD.Token.color.primary
    selectedTextColor: MD.Token.color.getOn(selectionColor)
    wrapMode: Text.Wrap
    textFormat: Text.PlainText
}
