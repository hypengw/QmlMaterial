import QtQuick
import Qcm.Material as MD

MD.CommandButtonBase {
    id: control
    property bool animationsEnabled: true
    property bool expanded: false
    property MD.springSpec shapeSpringSpec: MD.Token.motion.spring.standard.defaultEffects

    property MD.StateSplitButtonIndicator mdState: MD.StateSplitButtonIndicator {
        item: control
    }
    readonly property MD.ButtonMotion _motion: MD.ButtonMotion {
        source: control.mdState
        shapeSpringSpec: control.shapeSpringSpec
        enabled: control.animationsEnabled && control.visible && (!control.Window.window || (control.Window.window.visible && control.Window.window.visibility !== Window.Minimized))
    }

    Binding {
        control.mdState.selected: control.expanded
    }

    implicitWidth: mdState.sizeToken.trailing_button_leading_space + icon.width + mdState.sizeToken.trailing_button_trailing_space
    implicitHeight: mdState.sizeToken.container_height

    icon.width: mdState.sizeToken.trailing_button_icon_size
    icon.height: mdState.sizeToken.trailing_button_icon_size
    icon.color: control.mdState.textColor

    icon.name: MD.Token.icon.expand_more

    contentItem: MD.IconView {
        icon: control.icon
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: control.mdState.groupOpticalOffset(control._motion.corners)
        opacity: control.mdState.contentOpacity
        rotation: control.expanded ? 180 : 0
        Behavior on rotation {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
        }
    }

    background: MD.ElevationRectangle {
        corners: control._motion.corners
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
        opacity: control.mdState.backgroundOpacity
        MD.Ripple {
            anchors.fill: parent
            corners: parent.corners
            interactionSource: control.interactionSource
            indication: control.mdState.indication
            color: control.mdState.stateLayerColor
            animationsEnabled: control.animationsEnabled
        }
    }
}
