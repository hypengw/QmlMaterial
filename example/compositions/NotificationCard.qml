pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.Material as MD

MD.SwipeToDismiss {
    id: root
    required property string app
    required property string title
    required property string message
    required property string detail
    required property string age
    required property string symbol
    property url thumbnail: ''
    property bool ongoing: false
    property bool canReply: false
    property bool expanded: false
    property bool composing: false
    property bool firstInSection: true
    property bool lastInSection: true
    readonly property real collapsedHeight: 72
    readonly property real expandedContentTop: 16 + 10 + 12
    readonly property real expandedHeight: Math.max(collapsedHeight, expandedContentTop + titleLabel.height + 8 + expandedMessage.implicitHeight + (canReply ? 54 : 16))
    readonly property real targetHeight: expanded ? expandedHeight + (composing && canReply ? 64 : 0) : collapsedHeight
    readonly property real expansionFraction: expandedHeight > collapsedHeight ? Math.max(0, Math.min(1, (height - collapsedHeight) / (expandedHeight - collapsedHeight))) : expanded ? 1 : 0
    readonly property real contentFade: Math.min(1, expansionFraction * 360 / 210)
    readonly property real incomingOpacity: MD.Util.bezierY(contentFade, 0.4, 0, 1, 1)
    readonly property real outgoingOpacity: MD.Util.bezierY(1 - contentFade, 0, 0, 0.8, 1)
    readonly property real composeFraction: Math.max(0, Math.min(1, (height - expandedHeight) / 64))
    readonly property bool expansionAnimating: heightAnimation.running
    readonly property bool expansionMotionAllowed: __expansionComplete && animationsEnabled && visible && Window.window !== null && Window.window.visible && Window.window.visibility !== Window.Minimized
    property bool __expansionComplete: false
    property real __presentedHeight: collapsedHeight
    function __updateHeight() {
        if (!__expansionComplete) {
            __presentedHeight = targetHeight;
            return;
        }
        heightAnimation.stop();
        if (expansionMotionAllowed && __presentedHeight !== targetHeight) {
            heightAnimation.from = __presentedHeight;
            heightAnimation.to = targetHeight;
            heightAnimation.start();
        } else {
            __presentedHeight = targetHeight;
        }
    }
    onTargetHeightChanged: __updateHeight()
    // Policy changes may arrive while an animated height binding is being evaluated.
    onExpansionMotionAllowedChanged: Qt.callLater(__updateHeight)
    Component.onCompleted: {
        __presentedHeight = targetHeight;
        __expansionComplete = true;
        if (composing && composer.visible)
            replyInput.forceActiveFocus();
    }
    onExpandedChanged: if (!expanded)
        composing = false
    onComposingChanged: {
        if (!composing)
            replyInput.focus = false;
        else if (__expansionComplete && composer.visible)
            replyInput.forceActiveFocus();
    }
    signal dismissRequested
    signal replyRequested(string text)
    implicitWidth: 360
    implicitHeight: __presentedHeight
    startToEndEnabled: !ongoing && !composing
    endToStartEnabled: !ongoing && !composing
    onDismissed: dismissRequested()
    NumberAnimation {
        id: heightAnimation
        objectName: 'notificationHeightAnimation'
        target: root
        property: '__presentedHeight'
        duration: 360
        easing: MD.Token.easing.legacy
    }
    MD.Rectangle {
        width: root.width
        height: root.height
        clip: true
        corners: MD.Util.corners(root.firstInSection ? 28 : 4, root.firstInSection ? 28 : 4, root.lastInSection ? 28 : 4, root.lastInSection ? 28 : 4)
        color: Qt.rgba(root.MD.MProp.color.surface_container_high.r, root.MD.MProp.color.surface_container_high.g, root.MD.MProp.color.surface_container_high.b, 0.82)
        Rectangle {
            objectName: 'notificationAppIcon'
            x: 16
            y: 16
            width: 40
            height: 40
            radius: 20
            color: root.MD.MProp.color.secondary_container
            MD.Icon {
                anchors.centerIn: parent
                size: 24
                name: root.symbol
                color: root.MD.MProp.color.on_secondary_container
            }
        }
        MD.Text {
            objectName: 'notificationAppHeader'
            x: 72
            width: Math.max(0, parent.width - x - (thumbnailImage.visible ? 106 : 60))
            height: 48
            visible: opacity > 0
            opacity: root.incomingOpacity
            Accessible.ignored: !root.expanded
            text: root.app + ' - ' + root.age
            typescale: MD.Token.typescale.body_small
            color: root.MD.MProp.color.on_surface_variant
            wrapMode: Text.NoWrap
        }
        MD.IconButton {
            id: expander
            objectName: 'expandNotification'
            x: parent.width - width
            y: 0
            width: 58
            height: 50
            padding: 16
            topInset: 16
            bottomInset: 16
            leftInset: 16
            rightInset: 16
            icon.width: 16
            icon.height: 16
            icon.name: MD.Token.icon.expand_more
            mdState.type: MD.Enum.IBtFilledTonal
            mdState.corners: MD.Util.corners(9)
            mdState.elevation: 0
            Accessible.name: root.expanded ? 'Collapse notification' : 'Expand notification'
            contentItem: MD.IconView {
                icon: expander.icon
                rotation: 180 * root.expansionFraction
            }
            onClicked: {
                root.composing = false;
                root.expanded = !root.expanded;
            }
        }
        Row {
            objectName: 'notificationCollapsedHeader'
            x: 72
            y: 12
            width: Math.max(0, parent.width - x - (thumbnailImage.visible ? 106 : 60))
            height: 24
            spacing: 4
            opacity: root.outgoingOpacity
            visible: opacity > 0
            Accessible.ignored: root.expanded
            MD.Text {
                objectName: 'notificationCollapsedTitle'
                width: Math.min(implicitWidth, Math.max(0, parent.width - timeLabel.width - parent.spacing))
                height: parent.height
                text: root.title.length ? root.title : root.app
                typescale: root.title.length ? MD.Token.typescale.title_small : MD.Token.typescale.body_small
                font.weight: root.title.length ? Font.Bold : Font.Normal
                wrapMode: Text.NoWrap
            }
            MD.Text {
                id: timeLabel
                width: Math.min(implicitWidth, parent.width * 0.4)
                height: parent.height
                text: '- ' + root.age
                typescale: MD.Token.typescale.body_small
                color: root.MD.MProp.color.on_surface_variant
                wrapMode: Text.NoWrap
            }
        }
        MD.Image {
            id: thumbnailImage
            objectName: 'notificationThumbnail'
            x: parent.width - 90
            y: 12
            width: 48
            height: 48
            radius: 8
            source: root.thumbnail
            sourceSize: Qt.size(96, 96)
            visible: root.thumbnail.toString().length > 0
        }
        MD.Text {
            id: titleLabel
            objectName: 'notificationExpandedTitle'
            x: 72
            y: 12 + (root.expandedContentTop - 12) * root.expansionFraction
            width: Math.max(0, parent.width - x - (thumbnailImage.visible ? 106 : 16))
            height: root.title.length ? implicitHeight : 0
            text: root.title
            typescale: MD.Token.typescale.title_small
            font.weight: Font.Bold
            opacity: root.incomingOpacity
            visible: height > 0 && opacity > 0
            Accessible.ignored: !root.expanded
            wrapMode: Text.Wrap
            maximumLineCount: 2
        }
        MD.Text {
            objectName: 'notificationSummary'
            x: 72
            y: 36
            width: Math.max(0, parent.width - x - (thumbnailImage.visible ? 106 : 16))
            height: 24
            opacity: root.outgoingOpacity
            visible: opacity > 0
            Accessible.ignored: root.expanded
            text: root.message
            typescale: MD.Token.typescale.body_medium
            color: root.MD.MProp.color.on_surface_variant
            wrapMode: Text.NoWrap
        }
        MD.Text {
            id: expandedMessage
            objectName: 'notificationDetail'
            x: 72
            y: titleLabel.y + titleLabel.height + 8
            width: Math.max(0, parent.width - x - (thumbnailImage.visible && !root.title.length ? 106 : 16))
            opacity: root.incomingOpacity
            visible: opacity > 0
            Accessible.ignored: !root.expanded
            text: root.detail
            typescale: MD.Token.typescale.body_medium
            color: root.MD.MProp.color.on_surface_variant
            wrapMode: Text.Wrap
        }
        Row {
            x: 56
            y: expandedMessage.y + expandedMessage.implicitHeight
            height: 48
            spacing: 4
            visible: root.canReply && opacity > 0
            opacity: root.incomingOpacity
            enabled: root.expanded && root.expansionFraction === 1
            MD.Button {
                objectName: 'replyNotification'
                height: 48
                text: 'Reply'
                mdState.type: MD.Enum.BtText
                onClicked: {
                    root.composing = !root.composing;
                }
            }
            MD.Button {
                objectName: 'readNotification'
                height: 48
                text: 'Mark as read'
                mdState.type: MD.Enum.BtText
                onClicked: root.dismissRequested()
            }
        }
        Item {
            id: composer
            x: 16
            y: Math.max(root.expandedHeight - 4, parent.height - height - 12)
            width: parent.width - 32
            height: 56
            visible: opacity > 0
            opacity: root.composeFraction
            enabled: root.expanded && root.composing
            onVisibleChanged: if (root.__expansionComplete && visible && root.composing)
                replyInput.forceActiveFocus()
            MD.TextField {
                id: replyInput
                objectName: 'notificationReplyInput'
                width: parent.width - 56
                placeholderText: 'Reply'
                onAccepted: {
                    if (text.trim().length) {
                        root.replyRequested(text);
                        text = '';
                        root.composing = false;
                    }
                }
            }
            MD.IconButton {
                objectName: 'sendNotificationReply'
                x: parent.width - width
                width: 48
                height: 56
                icon.name: MD.Token.icon.send
                enabled: replyInput.text.trim().length > 0
                Accessible.name: 'Send reply'
                onClicked: replyInput.accepted()
            }
        }
    }
}
