import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Slider item

    // Prefer palette from attached Theme when present, otherwise fall back to global token palette.
    readonly property var palette: root.ctx ? root.ctx.color : MD.Token.color

    elevation: MD.Token.elevation.level0
    textColor: palette.on_primary
    backgroundColor: palette.primary
    supportTextColor: palette.on_primary
    stateLayerColor: "#00000000"

    property color trackColor: palette.primary
    property color trackOverlayColor: palette.primary
    property color trackInactiveColor: MD.Util.transparent(palette.on_surface, 0.3)
    property color trackMarkInactiveColor: MD.Util.transparent(palette.on_surface, 0.38)
    property color trackMarkColor: palette.on_primary

    property real trackOverlayOpacity: 0.12

    states: [
        State {
            name: "Disabled"
            when: !root.item.enabled
            PropertyChanges {
                root.textColor: root.palette.on_surface
                root.backgroundColor: root.palette.on_surface
                root.trackInactiveColor: root.palette.on_surface
                root.backgroundOpacity: 0.38
            }
        },
        State {
            name: "Pressed"
            when: root.item.pressed || root.item.visualFocus
            PropertyChanges {
                root.stateLayerColor: {
                    const c = root.palette.primary;
                    return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                }
            }
        },
        State {
            name: "Hovered"
            when: root.item.hovered
            PropertyChanges {
                root.stateLayerColor: {
                    const c = root.palette.primary;
                    return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                }
            }
        }
    ]
}
