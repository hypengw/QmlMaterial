import QtQuick
import Qcm.Material as MD

MD.PageIndicatorBase {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    spacing: 6

    delegate: Rectangle {
        implicitWidth: 8
        implicitHeight: 8

        radius: width / 2
        color: MD.Token.color.on_background
        opacity: index === control.currentIndex ? 0.95 : pressed ? 0.7 : 0.45

        required property int index
        required property bool pressed

        Behavior on opacity {
            OpacityAnimator {
                duration: MD.Token.duration.short2
                easing: MD.Token.easing.linear
            }
        }
    }

    contentItem: Row {
        spacing: control.spacing

        layoutDirection: control.layoutDirection
    }
}
