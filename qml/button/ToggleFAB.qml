pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property bool animationsEnabled: true
    property int animationDuration: MD.Token.duration.short4
    property MD.StateFAB mdState: MD.StateFAB {
        item: control
    }
    readonly property real checkedProgress: motion.progress
    readonly property var _tokens: MD.Token.fab_menu
    property bool _complete: false
    Component.onCompleted: _complete = true

    readonly property QtObject _motion: QtObject {
        id: motion
        property real progress: control.checked ? 1 : 0
        Behavior on progress {
            enabled: control._complete && control.animationsEnabled
            NumberAnimation {
                duration: control.animationDuration
                easing: MD.Token.easing.emphasized
            }
        }
    }

    checkable: true
    implicitWidth: _tokens.buttonSize
    implicitHeight: _tokens.buttonSize
    padding: 0
    icon.name: checkedProgress > 0.5 ? MD.Token.icon.close : MD.Token.icon.add
    icon.width: _tokens.iconSize + (_tokens.closeIconSize - _tokens.iconSize) * checkedProgress
    icon.height: icon.width
    icon.color: MD.Util.mixColor(mdState.ctx.color.on_primary_container, mdState.ctx.color.on_primary, checkedProgress)

    contentItem: MD.IconView {
        icon: control.icon
    }
    contentItemLayout: MD.Control.LayoutNone
    Binding {
        control.contentItem.x: (control.width - control.contentItem.width) / 2
        control.contentItem.y: (control.height - control.contentItem.height) / 2
    }

    background: MD.ElevationRectangle {
        radius: control._tokens.closedCorner + (height / 2 - control._tokens.closedCorner) * control.checkedProgress
        color: MD.Util.mixColor(control.mdState.ctx.color.primary_container, control.mdState.ctx.color.primary, control.checkedProgress)
        elevation: control.mdState.elevation
        elevationVisible: !control.flat && color.a > 0
        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            pressed: control.pressed
            pressX: control.pressX
            pressY: control.pressY
            color: control.icon.color
            stateOpacity: control.mdState.stateLayerOpacity
        }
        MD.FocusIndicator {
            corners: MD.Util.corners(parent.radius)
            active: control.visualFocus
        }
    }
}
