import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.Control {
    id: control

    property bool gap: false
    readonly property MD.Menu menu: ListView.view?.menu ?? null
    readonly property bool expressive: menu?.variant !== MD.Menu.Variant.Baseline

    focusPolicy: Qt.NoFocus
    padding: 0
    implicitHeight: gap && expressive ? MD.MenuMetrics.groupGap : (expressive ? 5 : 17)

    background: Item {
        Rectangle {
            visible: !control.gap || !control.expressive
            anchors.verticalCenter: parent.verticalCenter
            x: MD.MenuMetrics.horizontalPadding
            width: Math.max(0, parent.width - x * 2)
            height: 1
            color: control.MD.MProp.color.outline_variant
        }
    }
}
