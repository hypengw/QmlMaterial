import QtQuick
import Qcm.Material as MD

Transition {
    OpacityAnimator {
        from: 0.0
        to: 1.0
        duration: MD.Token.duration.medium4 * 0.3
        easing.type: Easing.BezierSpline
        easing.bezierCurve: MD.Token.easing.emphasized_decelerate.list
    }

    ScaleAnimator {
        from: 0.8
        to: 1.0
        duration: MD.Token.duration.medium4
        easing.type: Easing.BezierSpline
        easing.bezierCurve: MD.Token.easing.emphasized_decelerate.list
    }
}
