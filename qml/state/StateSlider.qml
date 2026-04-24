import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

MD.MState {
    id: root

    required property T.Slider item

    elevation: MD.Token.elevation.level0
    textColor: root.ctx.color.on_primary
    backgroundColor: root.ctx.color.primary
    supportTextColor: root.ctx.color.on_primary
    stateLayerColor: "#00000000"

    property color trackColor: root.backgroundColor
    property color trackOverlayColor: root.backgroundColor
    property color trackInactiveColor: root.ctx.color.surface_container_highest
    property color trackMarkInactiveColor: root.ctx.color.on_surface_variant
    property color trackMarkColor: root.supportTextColor

    property real trackOverlayOpacity: 0.12

    property int handleLineWidth: item.pressed || item.visualFocus ? 2 : 4
    property int handleWidth: 12
    property int handleHeight: 44

    Behavior on handleLineWidth {
        NumberAnimation {
            duration: MD.Token.duration.short2
        }
    }

    state: MD.Util.stateText(item.enabled, item.pressed || item.visualFocus, item.hovered, false)

    states: [
        State {
            name: "disabled"
            PropertyChanges {
                root.textColor: root.ctx.color.on_surface
                root.backgroundColor: root.ctx.color.on_surface
                root.trackInactiveColor: root.ctx.color.on_surface
                root.backgroundOpacity: 0.38
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                root.stateLayerColor: {
                    const c = root.ctx.color.primary;
                    return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                }
            }
        },
        State {
            name: "hovered"
            PropertyChanges {
                root.stateLayerColor: {
                    const c = root.ctx.color.primary;
                    return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                }
            }
        }
    ]
}
