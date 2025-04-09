import QtQuick
import Qcm.Material as MD
import QtQuick.Templates as T

MD.State {
    id: root

    elevation: item.scrolling ? MD.Token.elevation.level2 : MD.Token.elevation.level0
    textColor: root.ctx.color.on_surface
    backgroundColor: item.scrolling ? root.ctx.color.surface_container : root.ctx.color.surface  
    supportTextColor: root.ctx.color.on_surface_variant
    stateLayerOpacity: 0.0
    stateLayerColor: root.ctx.color.on_surface

    property T.Control item
    property MD.t_typescale typescale: MD.Token.typescale.title_large
    property int containerHeight: 64

    states: [
        State {
            name: "small"
            when: root.item.type === MD.Enum.AppBarSmall
            PropertyChanges {
            }
        },
        State {
            name: "meidium"
            when: root.item.type === MD.Enum.AppBarMedium
            PropertyChanges {
                // root.backgroundColor: root.ctx.color.surface
                root.typescale: MD.Token.typescale.headline_small
                root.elevation: MD.Token.elevation.level0
                root.containerHeight: 112
            }
        },
        State {
            name: "large"
            when: root.item.type === MD.Enum.AppBarLarge
            PropertyChanges {
                // root.backgroundColor: root.ctx.color.surface
                root.typescale: MD.Token.typescale.headline_medium
                root.elevation: MD.Token.elevation.level0
                root.containerHeight: 152
            }
        }
    ]
}
