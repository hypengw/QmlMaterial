pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.Button {
    id: root
    required property string title
    required property string status
    required property string symbol
    property bool active: false
    property bool available: true
    property bool dualTarget: true
    property bool toggleable: true
    property string sideSymbol: ''
    readonly property real bounce: bounceMotion.value * 8
    MD.PressMotion {
        id: bounceMotion
        objectName: 'tileBounceMotion'
        visibilityThreshold: 0.0125
        pressed: root.mdState.interactions.pressed
        active: root.available && root.visible && root.animationsEnabled && root.toggleable && !root.dualTarget && (!root.Window.window || (root.Window.window.visible && root.Window.window.visibility !== Window.Minimized))
    }
    MD.SpringMotion {
        id: radiusMotion
        targetValue: root.available && root.active ? 24 : root.height / 2
        enabled: root.animationsEnabled && root.visible && (!root.Window.window || (root.Window.window.visible && root.Window.window.visibility !== Window.Minimized))
        visibilityThreshold: 0.1
    }
    property real textBounceScale: 1
    SequentialAnimation {
        id: contentBounce
        objectName: 'tileContentBounce'
        NumberAnimation {
            target: root
            property: 'textBounceScale'
            to: 1.06
            duration: 167
            easing.type: Easing.Linear
        }
        NumberAnimation {
            target: root
            property: 'textBounceScale'
            to: 1
            duration: 167
            easing.type: Easing.BezierSpline
            easing.bezierCurve: [0.05, 0, 0, 1, 1, 1]
        }
    }
    function resetContentBounce() {
        contentBounce.stop();
        textBounceScale = 1;
    }
    onVisibleChanged: if (!visible)
        resetContentBounce()
    onAnimationsEnabledChanged: if (!animationsEnabled)
        resetContentBounce()
    onAvailableChanged: if (!available)
        resetContentBounce()
    readonly property color contentColor: !available ? Qt.rgba(MD.MProp.color.on_surface_variant.r, MD.MProp.color.on_surface_variant.g, MD.MProp.color.on_surface_variant.b, 0.38) : active && !dualTarget ? MD.MProp.color.on_primary : MD.MProp.color.on_surface
    enabled: available
    signal toggleRequested
    signal detailsRequested

    implicitWidth: 256
    implicitHeight: 72
    padding: 8
    leftPadding: 8
    rightPadding: dualTarget ? 8 : 12
    topPadding: 8
    bottomPadding: 8
    mdState.toggleable: !dualTarget && toggleable
    mdState.selected: active
    mdState.type: MD.Enum.BtText
    mdState.indicationColor: root.contentColor
    Accessible.name: title + (status.length ? ', ' + status : '')
    onClicked: {
        if (dualTarget)
            detailsRequested();
        else {
            toggleRequested();
            if (!toggleable && animationsEnabled)
                contentBounce.restart();
        }
    }
    background: Rectangle {
        radius: radiusMotion.value
        color: !root.available ? Qt.rgba(root.MD.MProp.color.surface.r, root.MD.MProp.color.surface.g, root.MD.MProp.color.surface.b, 0.18) : root.active && !root.dualTarget ? root.MD.MProp.color.primary : Qt.rgba(root.MD.MProp.color.surface_container_high.r, root.MD.MProp.color.surface_container_high.g, root.MD.MProp.color.surface_container_high.b, 0.82)
        MD.Ripple {
            objectName: 'tileRipple'
            anchors.fill: parent
            corners: MD.Util.corners(parent.radius)
            interactionSource: root.interactionSource
            indication: root.mdState.indication
            color: root.contentColor
        }
        MD.FocusIndicator {
            corners: MD.Util.corners(parent.radius)
            active: root.focusVisible
        }
    }
    contentItem: Item {
        MD.IconToggleButton {
            objectName: 'toggleButton'
            visible: root.dualTarget
            width: Math.min(56, parent.width)
            x: root.mirrored ? parent.width - width : 0
            height: 56
            padding: 0
            topInset: 0
            bottomInset: 0
            leftInset: 0
            rightInset: 0
            anchors.verticalCenter: parent.verticalCenter
            checked: root.active
            icon.name: root.symbol
            mdState.type: MD.Enum.IBtFilled
            icon.width: 28
            icon.height: 28
            mdState.elevation: 0
            mdState.corners: MD.Util.corners(root.available && root.active ? 16 : 50)
            animationsEnabled: root.animationsEnabled
            mdState.backgroundColor: !root.available ? root.MD.MProp.color.surface : root.active ? root.MD.MProp.color.primary : root.MD.MProp.color.surface_container_highest
            mdState.textColor: root.dualTarget && root.active && root.available ? root.MD.MProp.color.on_primary : root.contentColor
            mdState.backgroundOpacity: root.dualTarget ? 1 : 0
            mdState.contentOpacity: 1
            Accessible.name: root.title
            MD.ToolTip.text: (root.active ? 'Turn off ' : 'Turn on ') + root.title
            MD.ToolTip.visible: mdState.interactions.hovered
            onCheckedChangeRequested: root.toggleRequested()
        }
        MD.Icon {
            visible: !root.dualTarget
            width: Math.min(56, parent.width)
            x: root.mirrored ? parent.width - width : 0
            height: 56
            size: 28
            name: root.symbol
            color: root.contentColor
        }
        Item {
            id: details
            objectName: 'tileLabels'
            x: root.mirrored ? 0 : 62
            width: Math.max(0, parent.width - 62)
            height: parent.height
            Column {
                id: labels
                transform: Scale {
                    origin.x: root.mirrored ? labels.width : 0
                    origin.y: labels.height / 2
                    xScale: root.textBounceScale
                    yScale: root.textBounceScale
                }
                anchors.verticalCenter: parent.verticalCenter
                x: root.mirrored && root.sideSymbol.length ? 38 : 0
                width: Math.max(0, parent.width - (root.sideSymbol.length ? 38 : 0))
                spacing: 0
                MD.Text {
                    objectName: 'tileLabel'
                    width: parent.width
                    text: root.title
                    typescale: MD.Token.typescale.title_small
                    font.weight: Font.Bold
                    color: root.contentColor
                    wrapMode: Text.NoWrap
                    horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
                }
                MD.Text {
                    objectName: 'tileSecondaryLabel'
                    visible: text.length > 0
                    width: parent.width
                    text: root.status
                    color: root.contentColor
                    typescale: MD.Token.typescale.label_medium
                    wrapMode: Text.NoWrap
                    horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
                }
            }
            MD.Icon {
                objectName: 'tileChevron'
                anchors.right: root.mirrored ? undefined : parent.right
                anchors.left: root.mirrored ? parent.left : undefined
                anchors.verticalCenter: parent.verticalCenter
                visible: root.sideSymbol.length > 0
                name: root.sideSymbol
                color: root.contentColor
                size: 20
                width: 32
                height: 20
            }
        }
    }
    readonly property alias detailsAnchor: root
}
