import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    readonly property bool hasIcon: MD.Util.hasIcon(icon)
    property MD.StateAssistChip mdState: MD.StateAssistChip {
        item: control
    }

    property alias elevated: control.mdState.elevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: hasIcon ? 8 : 16
    rightPadding: 16
    spacing: 8

    icon.width: 18
    icon.height: 18
    font.capitalization: Font.Capitalize

    contentItem: MD.IconLabel {
        text: control.text
        typescale: MD.Token.typescale.label_large
        color: control.mdState.textColor

        icon_color: control.mdState.iconColor
        icon_name: control.icon.name
        icon_size: control.icon.width
        opacity: control.mdState.contentOpacity
    }

    background: MD.ElevationRectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: 1
        border.color: control.mdState.outlineColor
        elevation: control.mdState.elevation

        MD.Ripple2 {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

    MD.StateHolder {
        state: control.mdState
    }
}
