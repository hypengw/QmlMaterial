import QtQuick
import Qcm.Material as MD

MD.MState {
    id: root

    required property Item item
    property int type: MD.Enum.BottomSheetModal
    property int radius: MD.Token.shape.corner.extra_large
    property MD.corners corners: MD.Util.corners(radius, radius, 0, 0)

    elevation: MD.Token.elevation.level1
    textColor: root.ctx.color.on_surface
    backgroundColor: root.ctx.color.surface_container_low
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerColor: "transparent"
}
