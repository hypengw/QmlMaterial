import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 0.0
        to: 1.0
        duration: MD.Token.duration.long1
        easing: MD.Token.easing.emphasized
    }

    ScaleAnimator {
        from: 0.92
        to: 1.0
        duration: MD.Token.duration.long1
        easing: MD.Token.easing.emphasized
    }
}
