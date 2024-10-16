import QtQuick
import Qcm.Material as MD

MD.Rectangle {
    id: root
    property alias elevation: m_shadow.elevation
    property alias elevationItem: m_shadow
    property alias elevationVisible: m_shadow.visible

    MD.Elevation {
        id: m_shadow
        z: -1
        corners: root.corners
        width: root.width
        height: root.height
    }
}
