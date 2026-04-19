import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateSplitButtonIndicator mdState: MD.StateSplitButtonIndicator {
        item: control
    }
    
    property T.Menu menu: null

    implicitWidth: 32
    implicitHeight: 40

    icon.width: 18
    icon.height: 18
    
    checkable: true
    checked: control.menu ? control.menu.visible : false
    icon.name: checked ? MD.Token.icon.expand_less : MD.Token.icon.expand_more

    onClicked: {
        if (control.menu) {
            if (control.menu.visible) {
                control.menu.close();
            } else {
                // Position the menu below the whole SplitButton component
                // control.parent is Row, control.parent.parent is T.Control (SplitButton)
                let splitButton = control.parent.parent;
                
                // If menu parent is not set or is the SplitButton, we can set x/y directly
                // T.Menu positions are relative to its parent.
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

    contentItem: MD.Icon {
        name: control.icon.name
        size: control.icon.width
        color: control.mdState.textColor
        anchors.centerIn: parent
        opacity: control.mdState.contentOpacity
    }

    background: MD.ElevationRectangle {
        corners: control.mdState.corners
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
        
        MD.Ripple2 {
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
