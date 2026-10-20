pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ListItem {
    id: root
    objectName: "row"
    required index
    required model
    readonly property bool hasModel: visible && index >= 0 && !!model
    enabled: hasModel
    text: "Song"
    mdState.backgroundColor: mdState.ctx.color.surface_container
}
