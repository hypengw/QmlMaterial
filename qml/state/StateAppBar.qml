import QtQuick
import Qcm.Material as MD

MD.State {
    id: root

    elevation: item.onScroll ? MD.Token.elevation.level2 : MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerOpacity: 0.0
    stateLayerColor: root.ctx.color.on_surface
}
