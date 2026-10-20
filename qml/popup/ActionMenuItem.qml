import QtQuick
import Qcm.Material as MD

MD.MenuItem {
    property MD.Action action: null
    MD.ActionCommand.action: action
    text: MD.ActionCommand.text
    icon.actionIcon: MD.ActionCommand.icon
    enabled: MD.ActionCommand.enabled
    selected: action?.checked ?? false
    busy: action?.busy ?? MD.Enum.Idle
    progress: action?.progress ?? 0
    closeOnActivation: action?.closeMenu ?? true
}
