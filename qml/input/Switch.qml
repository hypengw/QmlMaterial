import QtQuick
import Qcm.Material as MD

MD.SwitchBase {
    id: control

    property alias mdState: m_sh
    property bool animationsEnabled: true
    property MD.springSpec springSpec: MD.Token.motion.spring.standard.fastSpatial
    readonly property bool motionEnabled: control.animationsEnabled && control.visible && control.enabled && (!control.Window.window || (control.Window.window.visible && control.Window.window.visibility !== Window.Minimized))

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 8
    topInset: 0
    bottomInset: 0
    rightInset: 0
    leftInset: 0

    icon.width: 16
    icon.height: 16
    icon.color: control.mdState.textColor

    indicator: Rectangle {
        id: indicator
        width: 52
        height: 32
        radius: height / 2
        y: parent.height / 2 - height / 2
        color: MD.Util.transparent(control.mdState.backgroundColor, control.mdState.backgroundOpacity)
        border.width: 2
        border.color: MD.Util.transparent(control.mdState.ctx.color.outline, control.mdState.backgroundOpacity)

        Behavior on color {
            ColorAnimation {
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.linear
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.linear
            }
        }

        Rectangle {
            id: handle
            objectName: 'switchHandle'
            opacity: control.mdState.contentOpacity
            x: (control.mirrored ? indicator.width - positionMotion.value : positionMotion.value) - width / 2
            y: (parent.height - height) / 2
            width: Math.max(0, sizeMotion.value)
            height: width
            radius: width / 2
            color: control.mdState.handleColor
            readonly property bool hasIcon: !control.icon.empty
            MD.SpringMotion {
                id: sizeMotion
                objectName: 'switchSizeMotion'
                targetValue: control.mdState.handleSize
                spec: control.springSpec
                enabled: control.motionEnabled && !control.mdState.interactions.pressed && !control.dragging
            }
            MD.SpringMotion {
                id: positionMotion
                objectName: 'switchPositionMotion'
                targetValue: indicator.height / 2 + control.position * Math.max(0, indicator.width - indicator.height)
                spec: control.springSpec
                enabled: control.motionEnabled && !control.dragging
            }

            Behavior on color {
                ColorAnimation {
                    duration: MD.Token.duration.short4
                    easing: MD.Token.easing.linear
                }
            }

            MD.IconView {
                anchors.centerIn: parent
                icon: control.icon
                visible: handle.hasIcon
            }
        }
        MD.Ripple {
            id: m_ripple
            x: handle.x + handle.width / 2 - width / 2
            y: handle.y + handle.height / 2 - height / 2
            width: 28
            height: 28
            radius: parent.radius
            interactionSource: control.interactionSource
            indication: control.mdState.indication
            animationsEnabled: control.animationsEnabled
            color: control.mdState.stateLayerColor
        }

        MD.FocusIndicator {
            parent: m_ripple
            corners: MD.Util.corners(m_ripple.width / 2)
            active: control.focusVisible
        }
    }

    contentItem: Item {
        implicitWidth: control.indicator.width
        implicitHeight: control.indicator.height
    }

    MD.StateSwitch {
        id: m_sh
        focusIndicationEnabled: false
        item: control
    }
}
