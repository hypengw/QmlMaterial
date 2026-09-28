pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property bool expanded: true
    property bool animationsEnabled: true
    property int expandDuration: MD.Token.duration.short4
    property int collapseDuration: MD.Token.duration.medium2
    property int fadeInDuration: MD.Token.duration.short4
    property int fadeOutDuration: MD.Token.duration.short2
    property real expansionProgress: motion.progress
    property real labelOpacity: motion.alpha
    readonly property bool transitioning: widthAnimation.running || alphaAnimation.running
    property real minimumExpandedWidth: MD.Token.extended_fab.minimumWidth
    property int color: MD.Enum.FABColorPrimary
    property MD.typescale typescale: MD.Token.typescale.label_large
    property MD.StateFAB mdState: MD.StateFAB {
        item: control
    }
    readonly property real expandedWidth: Math.max(minimumExpandedWidth, leftPadding + rightPadding + label.implicitWidth + (icon.empty ? 0 : icon.width + spacing))
    readonly property real _progress: icon.empty ? 1 : MD.Util.clamp(expansionProgress, 0, 1)
    readonly property real _collapsedWidth: MD.Token.extended_fab.height
    property bool _complete: false
    Component.onCompleted: _complete = true

    Binding {
        control.mdState.color: control.color
    }
    readonly property QtObject _motion: QtObject {
        id: motion
        property real progress: control.expanded || control.icon.empty ? 1 : 0
        property real alpha: control.expanded || control.icon.empty ? 1 : 0
        Behavior on progress {
            enabled: control._complete && control.animationsEnabled
            NumberAnimation {
                id: widthAnimation
                duration: control.expanded ? control.expandDuration : control.collapseDuration
                easing: MD.Token.easing.emphasized
            }
        }
        Behavior on alpha {
            enabled: control._complete && control.animationsEnabled
            NumberAnimation {
                id: alphaAnimation
                duration: control.expanded ? control.fadeInDuration : control.fadeOutDuration
                easing: MD.Token.easing.standard
            }
        }
    }

    implicitWidth: MD.Util.lerp(_collapsedWidth, expandedWidth, _progress)
    implicitHeight: MD.Token.extended_fab.height
    padding: 0
    leftPadding: icon.empty ? MD.Token.extended_fab.trailingSpace : MD.Token.extended_fab.leadingSpace
    rightPadding: MD.Token.extended_fab.trailingSpace
    spacing: MD.Token.extended_fab.iconSpacing
    font.pixelSize: typescale.size
    font.weight: typescale.weight
    font.letterSpacing: typescale.tracking
    icon.width: MD.Token.extended_fab.iconSize
    icon.height: MD.Token.extended_fab.iconSize
    icon.color: mdState.textColor

    contentItemLayout: MD.Control.LayoutNone
    contentItem: Item {
        width: control.width
        height: control.height
        clip: true
        readonly property real leading: MD.Util.lerp((control._collapsedWidth - image.width) / 2, control.leftPadding, control._progress)
        MD.IconView {
            id: image
            icon: control.icon
            x: control.mirrored ? parent.width - parent.leading - width : parent.leading
            anchors.verticalCenter: parent.verticalCenter
        }
        MD.Label {
            id: label
            readonly property real start: control.icon.empty ? control.leftPadding : parent.leading + image.width + control.spacing
            width: Math.max(0, Math.min(control.expandedWidth, control.width + (control.expandedWidth - control._collapsedWidth) * (1 - control._progress)) - start - control.rightPadding)
            x: control.mirrored ? parent.width - start - width : start
            anchors.verticalCenter: parent.verticalCenter
            text: control.text
            useTypescale: false
            font: control.font
            color: control.mdState.textColor
            opacity: control.icon.empty ? 1 : control.labelOpacity
            visible: opacity > 0
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
            Accessible.ignored: true
        }
    }
    background: MD.ElevationRectangle {
        radius: control.mdState.corner
        color: control.mdState.backgroundColor
        elevation: control.mdState.elevation
        elevationVisible: !control.flat && color.a > 0
        MD.Ripple {
            anchors.fill: parent
            radius: parent.radius
            color: control.mdState.stateLayerColor
            pressed: control.pressed
            pressX: control.pressX
            pressY: control.pressY
            stateOpacity: control.mdState.stateLayerOpacity
        }
        MD.FocusIndicator {
            corners: MD.Util.corners(parent.radius)
            active: control.visualFocus
        }
    }
}
