import QtQuick

import Qcm.Material as MD

MD.Scrollable {
    id: root

    clip: true

    synchronousDrag: true
    MD.NestedScroll.enabled: true
    MD.NestedScroll.pressDelay: MD.Token.flick.pressDelay
    maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
    flickDeceleration: MD.Token.flick.flickDeceleration

    leftMargin: 0
    rightMargin: 0
    topMargin: 0
    bottomMargin: 0
}
