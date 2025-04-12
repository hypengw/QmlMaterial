import QtQuick
import Qcm.Material as MD

MD.ElevationImpl {
    id: root
    color: MD.MProp.color.shadow
    visible: elevation != MD.Token.elevation.level0

    Behavior on elevation {
        NumberAnimation {
            duration: 100
            // easing.type: Easing.InOutCubic
        }
    }
}
