import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.PageIndicator {
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
        // opacity: control.enabled ? 1.0 : 0.38

        // qmllint disable unqualified
        // We can't make "pressed" a required property, as QQuickPageIndicator doesn't create
        // the delegates, and so it can't set it as an initial property.
        opacity: index === control.currentIndex ? 0.95 : pressed ? 0.7 : 0.45

        required property int index

        Behavior on opacity {
            OpacityAnimator {
                duration: 100
            }
        }
    }

    contentItem: Row {
        spacing: control.spacing

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }
}
