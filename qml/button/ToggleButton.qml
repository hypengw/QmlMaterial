import QtQuick
import Qcm.Material as MD

MD.ToggleButtonBase {
    id: control
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: !icon.empty
    property bool flat: mdState.type == MD.Enum.BtText || mdState.type == MD.Enum.BtOutlined
    property bool animationsEnabled: true
    property MD.springSpec shapeSpringSpec: MD.Token.motion.spring.standard.defaultEffects
    property MD.StateButtonBase mdState: MD.StateButton {
        focusIndicationEnabled: false
        item: control
    }
    Binding {
        control.mdState.selected: control.checked
        control.mdState.toggleable: true
    }
    readonly property MD.ButtonMotion _motion: MD.ButtonMotion {
        source: control.mdState
        shapeSpringSpec: control.shapeSpringSpec
        enabled: control.animationsEnabled && control.visible && (!control.Window.window || (control.Window.window.visible && control.Window.window.visibility !== Window.Minimized))
    }
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: mdState.leadingSpace
    rightPadding: mdState.trailingSpace
    spacing: mdState.spacing
    MD.ButtonGroupContainer.defaultCompressionLimit: Math.min(leftPadding, rightPadding)
    MD.ButtonGroupContainer.defaultMinimumWidth: implicitContentWidth
    icon.width: mdState.iconSize
    icon.height: mdState.iconSize
    icon.color: control._motion.textColor
    property MD.typescale typescale: MD.Token.typescale.label_large
    font.capitalization: Font.MixedCase
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking
    contentItem: MD.ButtonContent {
        button: control
        mdState: control.mdState
        motion: control._motion
        iconStyle: control.iconStyle
        typescale: control.typescale
    }
    background: MD.ButtonSurface {
        button: control
        mdState: control.mdState
        motion: control._motion
        implicitWidth: 64
        implicitHeight: control.mdState.containerHeight
        outlined: control.mdState.type == MD.Enum.BtOutlined
        border.color: control.enabled ? control.mdState.ctx.color.outline : control.mdState.ctx.color.on_surface
        flat: control.flat
    }
}
