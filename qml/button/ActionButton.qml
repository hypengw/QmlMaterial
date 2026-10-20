import QtQuick
import Qcm.Material as MD

MD.Button {
    id: control
    property MD.Action action: null
    MD.ActionCommand.action: action
    text: MD.ActionCommand.text
    icon.actionIcon: MD.ActionCommand.icon
    enabled: MD.ActionCommand.enabled
    Binding {
        control.mdState.selected: control.action?.checked ?? false
        control.mdState.toggleable: control.action?.checkable ?? false
    }
}
