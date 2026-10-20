import QtQuick
import Qcm.Material as MD

MD.CommandButtonBase {
    id: root
    property MD.typescale typescale: MD.Token.typescale.body_medium
    property alias color: label.color
    property alias maximumLineCount: label.maximumLineCount
    property alias verticalAlignment: label.verticalAlignment
    property MD.Action action: null
    MD.ActionCommand.action: action
    text: MD.ActionCommand.text
    enabled: MD.ActionCommand.enabled
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    Accessible.role: Accessible.Button
    Accessible.name: text
    Accessible.onPressAction: click()

    MD.InteractionObserver {
        id: interactions
        source: root.interactionSource
    }
    contentItem: MD.Label {
        id: label
        text: root.text
        typescale: root.typescale
        font: root.font
        color: interactions.values.hovered ? root.MD.MProp.color.primary : root.MD.MProp.color.on_background
    }
}
