pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.FABMenuBase {
    id: control

    property int sequenceDuration: MD.Token.duration.medium4
    revealCount: expanded ? visibleCount : 0
    sequencing: sequenceAnimation.running

    Behavior on revealCount {
        enabled: control.motionEnabled && control.animationsEnabled
        NumberAnimation {
            id: sequenceAnimation
            duration: control.sequenceDuration
            easing: MD.Token.easing.standard
        }
    }

    button: MD.ToggleFAB {
        checked: control.expanded
        checkable: false
        animationsEnabled: control.animationsEnabled
        onClicked: control.toggle()
    }
}
