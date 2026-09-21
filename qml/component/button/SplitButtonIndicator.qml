import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property MD.StateSplitButtonIndicator mdState: MD.StateSplitButtonIndicator {
        item: control
    }
    
    property MD.MenuBase menu: null

    implicitWidth: 32
    implicitHeight: 40

    icon.width: 18
    icon.height: 18
    icon.color: control.mdState.textColor
    
    checkable: true
    checked: control.menu ? control.menu.visible : false
    icon.name: checked ? MD.Token.icon.expand_less : MD.Token.icon.expand_more

    onClicked: {
        if (!control.enabled) return;
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
        icon: control.icon
        anchors.centerIn: parent
         opacity: control.mdState.contentOpacity
    }

    background: MD.ElevationRectangle {
        corners: control.mdState.corners
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
