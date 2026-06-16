import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root
    required property T.Control item

    readonly property real padding: 0
    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface
    stateLayerColor: "transparent"
}
