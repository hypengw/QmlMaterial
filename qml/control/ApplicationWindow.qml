import QtQuick
import Qcm.Material as MD

Window {
    id: window

    default property alias contentData: body.contentData
    readonly property Item contentItem: body.contentItem
    property alias background: frame.background
    property alias menuBar: frame.header
    property alias header: body.header
    property alias footer: body.footer
    property alias font: frame.font
    property alias locale: frame.locale
    property alias topPadding: body.topPadding
    property alias bottomPadding: body.bottomPadding
    property alias leftPadding: body.leftPadding
    property alias rightPadding: body.rightPadding
    readonly property Item activeFocusControl: {
        const control = MD.Util.controlAncestor(activeFocusItem);
        return control === frame || control === body ? null : control;
    }
    color: MD.MProp.backgroundColor

    data: MD.PageBase {
        id: frame
        anchors.fill: parent
        focus: true
        contentItem: MD.PageBase {
            id: body
            focus: true
        }
    }

    property MD.typescale typescale: MD.Token.typescale.body_medium
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking
}
