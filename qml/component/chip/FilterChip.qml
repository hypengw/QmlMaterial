import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Button {
    id: control

    property MD.StateFilterChip mdState: MD.StateFilterChip {
        item: control
    }

    property alias elevated: control.mdState.elevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: true
    checkable: !action
    checked: action
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    verticalPadding: 0
    leftPadding: control.checked ? 8 : 16
    rightPadding: MD.Util.hasIcon(icon) ? 8 : 16
    spacing: 8

    Behavior on leftPadding {
        NumberAnimation { duration: 50 }
    }

    icon.width: 18
    icon.height: 18
    font.capitalization: Font.Capitalize

    contentItem: Row {
        opacity: control.mdState.contentOpacity
        Item {
            id: m_leading
            anchors.verticalCenter: parent.verticalCenter
            // Direct bindings (not State.PropertyChanges) so the leading-icon
            // width tracks the inner Row even for chips that are unchecked at
            // construction. PropertyChanges captures the binding's *value* at
            // state-entry time, which for chips first entering "hidden" before
            // their children exist froze implicitWidth at 0 forever — every
            // chip past index 0 in a Repeater never showed its check icon.
            //
            // `visible` is driven by `implicitWidth` (animated on the GUI
            // thread) rather than `opacity` — OpacityAnimator runs on the
            // render thread and never ticks the QML-side opacity value, so
            // an opacity-derived visible would only flip at the animation
            // edges and the icon would pop instead of fade.
            implicitWidth: control.checked ? m_leading_inner.implicitWidth : 0
            implicitHeight: m_leading_inner.implicitHeight
            opacity: control.checked ? 1 : 0
            visible: implicitWidth > 0

            Behavior on implicitWidth {
                NumberAnimation { duration: 100 }
            }
            Behavior on opacity {
                OpacityAnimator { duration: 100 }
            }

            Row {
                id: m_leading_inner
                spacing: 0
                MD.Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: MD.Token.icon.check
                    size: control.icon.width
                    color: control.mdState.leadingIconColor
                }
                MD.Space {
                    spacing: control.spacing
                }
            }
        }
        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            MD.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                verticalAlignment: Text.AlignVCenter
                typescale: MD.Token.typescale.label_large
            }
            MD.Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: name
                name: control.icon.name
                size: control.icon.width
                color: control.mdState.trailingIconColor
            }
        }
    }

    background: MD.ElevationRectangle {
        implicitWidth: 32
        implicitHeight: 32

        radius: 8
        color: control.mdState.backgroundColor
        opacity: control.mdState.backgroundOpacity

        border.width: control.checked ? 0 : 1
        border.color: control.mdState.outlineColor
        elevation: control.mdState.elevation

        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            pressX: control.pressX
            pressY: control.pressY
            pressed: control.pressed
            stateOpacity: control.mdState.stateLayerOpacity
            color: control.mdState.stateLayerColor
        }
    }

}
