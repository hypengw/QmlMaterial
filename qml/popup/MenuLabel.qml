import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.Control {
    id: control

    property string text
    readonly property MD.Menu menu: ListView.view?.menu ?? null

    focusPolicy: Qt.NoFocus
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: MD.MenuMetrics.labelHeight
    horizontalPadding: MD.MenuMetrics.horizontalPadding + MD.MenuMetrics.horizontalInset
    verticalPadding: 0

    contentItem: MD.Label {
        text: control.text
        typescale: MD.Token.typescale.label_large
        color: control.menu?.mdState.supportTextColor ?? control.MD.MProp.color.on_surface_variant
        wrapMode: Text.NoWrap
    }
}
