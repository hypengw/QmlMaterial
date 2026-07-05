pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Button {
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
    contentItem.visible: !busy

    MD.Loader {
        id: m_loader
        anchors.centerIn: parent
        opacity: control.mdState.contentOpacity
        active: control.busy !== MD.Enum.Idle
        sourceComponent: comp_busy
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
                color: control.mdState.textColor
            }
        }
    }
}
