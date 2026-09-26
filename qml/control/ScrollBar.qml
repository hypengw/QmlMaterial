import QtQuick
import Qcm.Material as MD

/** @ingroup control */
MD.ScrollBarBase {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: control.interactive ? 1 : 2
    visible: control.policy !== MD.ScrollBarBase.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? (width > 0 ? height / width : 0) : (height > 0 ? width / height : 0)

    interactive: hovered || pressed

    contentItem: Rectangle {
        implicitWidth: control.interactive ? 6 : 2
        implicitHeight: control.interactive ? 6 : 2

        radius: control.orientation === Qt.Horizontal ? height / 2.0 : width / 2.0

        color: MD.Util.transparent(MD.Token.color.on_surface, control.pressed ? 0.8 : 0.38)
        opacity: 0.0
    }

    background: Rectangle {
        implicitWidth: control.interactive ? 8 : 2
        implicitHeight: control.interactive ? 8 : 2
        color: MD.Util.transparent(MD.Token.color.on_surface, 0.12)
        radius: control.orientation === Qt.Horizontal ? height / 2.0 : width / 2.0
        opacity: 0.0
        visible: control.interactive
    }

    states: [
        State {
            name: "active"
            when: control.policy === MD.ScrollBarBase.AlwaysOn || (control.active && control.size < 1.0)
        }
    ]

    transitions: [
        Transition {
            to: "active"
            NumberAnimation {
                duration: MD.Token.duration.medium1
                easing: MD.Token.easing.linear
                targets: [control.contentItem, control.background]
                property: "opacity"
                to: 1.0
            }
        },
        Transition {
            from: "active"
            SequentialAnimation {
                PropertyAction {
                    targets: [control.contentItem, control.background]
                    property: "opacity"
                    value: 1.0
                }
                PauseAnimation {
                    duration: 2450
                }
                NumberAnimation {
                    duration: MD.Token.duration.medium1
                    easing: MD.Token.easing.linear
                    targets: [control.contentItem, control.background]
                    property: "opacity"
                    to: 0.0
                }
            }
        }
    ]
}
