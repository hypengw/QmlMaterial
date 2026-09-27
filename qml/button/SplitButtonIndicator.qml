import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property MD.StateSplitButtonIndicator mdState: MD.StateSplitButtonIndicator {
        item: control
    }

    property MD.MenuBase menu: null

    implicitWidth: mdState.sizeToken.trailing_button_leading_space + icon.width + mdState.sizeToken.trailing_button_trailing_space
    implicitHeight: mdState.sizeToken.container_height

    icon.width: mdState.sizeToken.trailing_button_icon_size
    icon.height: mdState.sizeToken.trailing_button_icon_size
    icon.color: control.mdState.textColor

    checkable: true
    checked: control.menu ? control.menu.visible : false
    icon.name: MD.Token.icon.expand_more

    onClicked: {
        if (!control.enabled)
            return;
        if (control.menu) {
            if (control.menu.visible) {
                control.menu.close();
            } else {
                // Position the menu below the whole SplitButton component
                // control.parent is Row, control.parent.parent is SplitButton.
                let splitButton = control.parent.parent;

                // If menu parent is not set or is the SplitButton, we can set x/y directly
                // Menu coordinates are relative to its logical parent.
                if (!control.menu.parent) {
                    control.menu.parent = splitButton;
                }

                // Align with the left edge of the SplitButton by default
                control.menu.x = 0;
                control.menu.y = splitButton.height;

                control.menu.open();
            }
        }
    }

    contentItem: MD.IconView {
        property real opticalOffset: control.checked && !control.down ? 0 : control.mdState.sizeToken.trailing_button_optical_offset
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
        rotation: control.checked ? 180 : 0
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
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }
}
