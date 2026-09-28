pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.FABMenuItemBase {
    id: control

    property bool animationsEnabled: true
    property int spatialDuration: MD.Token.duration.short4
    property int effectDuration: MD.Token.duration.short2
    property MD.StateFAB mdState: MD.StateFAB {
        item: control
    }
    property MD.typescale typescale: MD.Token.typescale.title_medium
    readonly property var _tokens: MD.Token.fab_menu

    flat: true
    opacity: alphaProgress
    focusPolicy: inputEnabled ? Qt.StrongFocus : Qt.NoFocus
    Accessible.ignored: !inputEnabled
    padding: 0
    horizontalPadding: _tokens.itemPadding
    spacing: _tokens.iconSpacing
    implicitWidth: Math.max(_tokens.itemHeight, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(_tokens.itemHeight, implicitContentHeight + topPadding + bottomPadding)
    icon.width: _tokens.iconSize
    icon.height: _tokens.iconSize
    icon.color: mdState.textColor
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking

    widthProgress: revealed ? 1 : 0
    alphaProgress: revealed ? 1 : 0
    animating: widthAnimation.running || alphaAnimation.running
    Behavior on widthProgress {
        enabled: control.motionEnabled && control.animationsEnabled
        NumberAnimation {
            id: widthAnimation
            duration: control.spatialDuration
            easing: MD.Token.easing.emphasized
        }
    }
    Behavior on alphaProgress {
        enabled: control.motionEnabled && control.animationsEnabled
        NumberAnimation {
            id: alphaAnimation
            duration: control.effectDuration
            easing: MD.Token.easing.standard
        }
    }

    contentItem: Item {
        implicitWidth: label.implicitWidth + (control.icon.empty ? 0 : control.icon.width + control.spacing)
        implicitHeight: Math.max(label.implicitHeight, control.icon.height)
        clip: true

        Item {
            width: Math.max(0, (control.fullWidth || control.width) - control.leftPadding - control.rightPadding)
            height: parent.height
            x: control.alignRight ? parent.width - width : 0
            MD.IconView {
                id: image
                visible: !control.icon.empty
                icon: control.icon
                x: control.mirrored ? parent.width - width : 0
                anchors.verticalCenter: parent.verticalCenter
            }
            MD.Label {
                id: label
                x: control.mirrored || control.icon.empty ? 0 : image.width + control.spacing
                width: Math.max(0, parent.width - (control.icon.empty ? 0 : image.width + control.spacing))
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                useTypescale: false
                font: control.font
                color: control.mdState.textColor
                elide: Text.ElideRight
                wrapMode: Text.NoWrap
            }
        }
    }

    background: Rectangle {
        radius: height / 2
        color: control.mdState.backgroundColor
        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            pressed: control.pressed
            pressX: control.pressX
            pressY: control.pressY
            color: control.mdState.stateLayerColor
            stateOpacity: control.mdState.stateLayerOpacity
        }
        MD.FocusIndicator {
            corners: MD.Util.corners(parent.radius)
            active: control.visualFocus
        }
    }
}
