import QtQuick
import Qcm.Material as MD
import QtQuick.Templates as T

MD.State {
    id: root
    ctx: item.MD.MProp
    required property T.ToolBar item
    property int type: MD.Enum.AppBarCenterAligned
    property bool scrolling: false

    property MD.t_typescale typescale: MD.Token.typescale.title_large
    property int containerHeight: 64

    elevation: root.scrolling ? MD.Token.elevation.level2 : MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: root.scrolling ? root.ctx.color.surface_container : root.ctx.color.surface
    backgroundOpacity: root.scrolling ? 1 : 0
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerOpacity: 0.0
    stateLayerColor: root.ctx.color.on_surface

    states: [
        State {
            name: "small"
            when: root.type === MD.Enum.AppBarSmall
            PropertyChanges {}
        },
        State {
            name: "meidium"
            when: root.type === MD.Enum.AppBarMedium
            PropertyChanges {
                // root.backgroundColor: root.ctx.color.surface
                root.typescale: MD.Token.typescale.headline_small
                root.elevation: MD.Token.elevation.level0
                root.containerHeight: 112
            }
        },
        State {
            name: "large"
            when: root.type === MD.Enum.AppBarLarge
            PropertyChanges {
                // root.backgroundColor: root.ctx.color.surface
                root.typescale: MD.Token.typescale.headline_medium
                root.elevation: MD.Token.elevation.level0
                root.containerHeight: 152
            }
        }
    ]
}
