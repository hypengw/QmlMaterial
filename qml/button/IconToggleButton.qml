import QtQuick
import Qcm.Material as MD

MD.ToggleButtonBase {
    id: control
    property bool flat: mdState.type == MD.Enum.IBtStandard || (mdState.type == MD.Enum.IBtOutlined && !mdState.selected)
    property bool animationsEnabled: true
    property MD.springSpec shapeSpringSpec: MD.Token.motion.spring.standard.defaultEffects
    property MD.StateIconButton mdState: MD.StateIconButton {
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
    topInset: MD.ButtonGroupContainer.connected ? 0 : 4
    bottomInset: MD.ButtonGroupContainer.connected ? 0 : 4
    leftInset: MD.ButtonGroupContainer.connected ? 0 : 4
    rightInset: MD.ButtonGroupContainer.connected ? 0 : 4
    padding: 8
    spacing: 0
    MD.ButtonGroupContainer.defaultCompressionLimit: Math.max(0, (mdState.containerWidth - mdState.iconSize) / 2)
    MD.ButtonGroupContainer.defaultMinimumWidth: mdState.iconSize + leftInset + rightInset
    icon.width: mdState.iconSize
    icon.height: mdState.iconSize
    icon.color: control._motion.textColor
    icon.fill: checked
    contentItem: MD.IconButtonContent {
        button: control
        mdState: control.mdState
        motion: control._motion
    }
    background: MD.ButtonSurface {
        button: control
        mdState: control.mdState
        motion: control._motion
        implicitWidth: control.mdState.containerWidth
        implicitHeight: control.mdState.containerHeight
        outlined: control.mdState.type == MD.Enum.IBtOutlined
        flat: control.flat
    }
}
