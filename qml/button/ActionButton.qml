import QtQuick
import Qcm.Material as MD

MD.Button {
    id: control
    property MD.Action action: null
    shapeSpringSpec: mdState.toggleable ? MD.Token.motion.spring.standard.fastSpatial : MD.Token.motion.spring.standard.defaultEffects
    mdState.shapeMode: mdState.toggleable ? MD.Enum.InteractiveShape : MD.Enum.FixedShape
    MD.ActionCommand.action: action
    text: MD.ActionCommand.text
    icon.actionIcon: MD.ActionCommand.icon
    enabled: MD.ActionCommand.enabled
    Binding {
        control.mdState.selected: control.action?.checked ?? false
        control.mdState.toggleable: control.action?.checkable ?? false
    }
}
