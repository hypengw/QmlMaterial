import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 1.0
        to: 0.0
        duration: MD.Token.duration.long1
        easing: MD.Token.easing.emphasized
    }

    ScaleAnimator {
        from: 1.0
        to: 0.92
        duration: MD.Token.duration.long1
        easing: MD.Token.easing.emphasized
    }
}
