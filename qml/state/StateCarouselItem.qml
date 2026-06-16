import QtQuick
import Qcm.Material as MD

MD.MState {
    id: root
    required property Item item

    elevation: MD.Token.elevation.level1
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface_container_low
    stateLayerColor: root.ctx.color.on_surface

    state: MD.Util.stateText(item.enabled, item.down, item.hovered, item.activeFocus)
}
