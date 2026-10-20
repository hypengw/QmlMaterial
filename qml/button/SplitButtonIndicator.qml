import QtQuick
import Qcm.Material as MD

MD.MenuButtonBase {
    id: control

    property MD.StateSplitButtonIndicator mdState: MD.StateSplitButtonIndicator {
        item: control
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
        property real opticalOffset: control.expanded && !control.mdState.interactions.pressed ? 0 : control.mdState.sizeToken.trailing_button_optical_offset
        icon: control.icon
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -opticalOffset
        Behavior on opticalOffset {
            NumberAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
        }
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
        corners: control.mdState.corners
        Behavior on corners {
            PropertyAnimation {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.standard
            }
        }
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
        opacity: control.mdState.backgroundOpacity
        MD.Ripple {
            anchors.fill: parent
            corners: parent.corners
            interactionSource: control.interactionSource
            indication: control.mdState.indication
            color: control.mdState.stateLayerColor
        }
    }
}
