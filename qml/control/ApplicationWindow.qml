import QtQuick
import Qcm.Material as MD

MD.ApplicationWindowBase {
    id: window

    frameItem.focus: true
    bodyItem.focus: true

    color: MD.MProp.backgroundColor

    property MD.typescale typescale: MD.Token.typescale.body_medium
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking
}
