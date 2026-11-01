import QtQuick
import Qcm.Material as MD

/** @ingroup component */
MD.SkeletonBase {
    id: root
    color: MD.MProp.color.surface_container_highest
    radius: MD.Token.shape.corner.small
    Accessible.ignored: true

    Rectangle {
        anchors.fill: parent
        visible: !root.supported
        color: root.color
        radius: root.radius
    }
}
