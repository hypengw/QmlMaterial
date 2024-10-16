import QtQuick
import Qcm.Material as MD

MD.ElevationRectangle {
    id: root
    property alias border: m_border.border

    Rectangle {
        id: m_border
        radius: root.corners.radius()
        width: root.width
        height: root.height
        color: 'transparent'
    }
}
