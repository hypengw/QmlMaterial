import QtQuick
import QtQuick.Controls.Basic
import Qcm.Material as MD

GridView {
    id: root

    clip: true

    synchronousDrag: true
    pressDelay: 100
    maximumFlickVelocity: 999999
    flickDeceleration: 1000
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    rebound: Transition {}

    signal wheelMoved
    MD.WheelHandler {
        id: wheel
        target: root
        filterMouseEvents: false
        onWheelMoved: root.wheelMoved()
    }
    ScrollBar.vertical: MD.ScrollBar {
    }
}
