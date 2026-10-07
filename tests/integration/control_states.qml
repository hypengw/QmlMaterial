pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

Item {
    id: root
    required property int index
    required property var model
    readonly property bool hasModel: visible && index >= 0 && !!model
    enabled: hasModel
    implicitHeight: 160

    MD.TextField {
        id: field
        width: 180
        placeholderText: "Song"
        mdState.placeholderColor: field.mdState.ctx.color.primary
    }
    MD.ComboBox {
        id: combo
        x: 190
        width: 180
        model: ["One", "Two"]
        mdState.labelColor: combo.mdState.ctx.color.primary
    }
    MD.Slider {
        id: slider
        y: 70
        width: 240
        mdState.backgroundColor: slider.mdState.ctx.color.surface_container
    }
    MD.DragHandle {
        id: handle
        y: 120
        width: 50
        mdState.textColor: handle.mdState.ctx.color.primary
    }
    MD.SplitButtonIndicator {
        id: indicator
        x: 280
        y: 70
        mdState.backgroundColor: indicator.mdState.ctx.color.surface_container
    }
}
