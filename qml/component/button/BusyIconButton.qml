pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.IconButton {
    id: control
    property int busy: {
        if (action instanceof MD.Action)
            return (action as MD.Action).busy;
        return MD.Enum.Idle;
    }
    property real progress: {
        if (action instanceof MD.Action)
            return (action as MD.Action).progress;
        return 0;
    }
    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        opacity: control.mdState.contentOpacity

        MD.Loader {
            anchors.centerIn: parent
            sourceComponent: control.busy !== MD.Enum.Idle ? comp_busy : comp_icon
        }

        Component {
            id: comp_icon
            MD.Icon {
                name: control.icon.name
                size: Math.min(control.icon.width, control.icon.height)
                color: control.mdState.textColor
                fill: control.checked
            }
        }

        Component {
            id: comp_busy
            MD.CircularIndicator {
                anchors.centerIn: parent
                indeterminate: control.busy !== MD.Enum.Progress
                running: control.busy !== MD.Enum.Progress
                value: control.progress
                strokeWidth: 2
                padding: 0
                implicitWidth: {
                    const w = Math.min(control.icon.width, control.icon.height);
                    return w - 6;
                }
                implicitHeight: implicitWidth
            }
        }
    }
}
