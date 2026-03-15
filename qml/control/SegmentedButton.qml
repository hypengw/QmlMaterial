import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property int position: MD.Enum.PosSingle
    property alias mdState: m_state

    MD.StateSegmentedButton {
        id: m_state
        item: control
        position: control.position
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 12
    leftPadding: {
        if (position == MD.Enum.PosFirst || position == MD.Enum.PosSingle) {
            return 16;
        }
        return 12;
    }
    rightPadding: {
        if (position == MD.Enum.PosLast || position == MD.Enum.PosSingle) {
            return 16;
        }
        return 12;
    }
    spacing: 8
    
    checkable: true

    icon.width: 18
    icon.height: 18

    property MD.typescale typescale: MD.Token.typescale.label_large
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    z: checked ? 1 : 0

    contentItem: MD.IconLabel {
        text: control.text
        color: control.mdState.textColor
        style: (control.checked || MD.Util.hasIcon(control.icon)) ? MD.Enum.IconAndText : MD.Enum.TextOnly
        icon.name: control.checked ? "check" : control.icon.name
        icon.size: control.icon.width
        opacity: control.mdState.contentOpacity
        label.lineHeight: control.typescale.line_height
    }

    background: MD.Rectangle {
        implicitWidth: 40
        implicitHeight: 40

        corners: control.mdState.corners
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: 1
        border.color: control.mdState.outlineColor
        
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
