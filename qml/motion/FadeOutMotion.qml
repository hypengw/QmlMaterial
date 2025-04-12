import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 1.0
        to: 0.0
        duration: MD.Token.duration.short3
        easing.type: Easing.BezierSpline
        easing.bezierCurve: MD.Token.easing.emphasized_accelerate.list
    }

    ScaleAnimator {
        from: 1.0
        to: 0.8
        duration: MD.Token.duration.short3
        easing.type: Easing.BezierSpline
        easing.bezierCurve: MD.Token.easing.emphasized_accelerate.list
    }
}
