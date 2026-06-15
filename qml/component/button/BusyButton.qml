pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Button {
    id: control
    property bool busy: {
        if (action instanceof MD.Action) {
            return (action as MD.Action).busy;
        }
        return false;
    }
    contentItem.visible: !busy

    MD.Loader {
        id: m_loader
        anchors.centerIn: parent
        opacity: control.mdState.contentOpacity
        active: control.busy
        sourceComponent: comp_busy
        Component {
            id: comp_busy
            MD.CircularIndicator {
                anchors.centerIn: parent
                running: true
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
