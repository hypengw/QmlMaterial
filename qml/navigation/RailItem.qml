import QtQuick
import Qcm.Material as MD

MD.NavigationItemBase {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    property int collapsedIconStyle: iconStyle
    property bool expand: false
    property real expansionProgress: -1
    property Item trailing: null
    property bool animationsEnabled: true
    property MD.springSpec selectionSpringSpec: MD.Token.motion.spring.standard.defaultSpatial
    property MD.springSpec effectsSpringSpec: MD.Token.motion.spring.standard.defaultEffects
    readonly property bool _motionEnabled: animationsEnabled && visible && (!Window.window || (Window.window.visible && Window.window.visibility !== Window.Minimized))
    readonly property MD.SpringMotion _selectionMotion: MD.SpringMotion {
        enabled: control._motionEnabled
        targetValue: control.selected ? 1 : 0
        spec: control.selectionSpringSpec
    }
    readonly property MD.ColorSpringMotion _collapsedLabelMotion: MD.ColorSpringMotion {
        enabled: control._motionEnabled && m_collapsed_label.visible
        targetColor: control.mdState.collapsedLabelColor
        spec: control.effectsSpringSpec
    }
    readonly property MD.ColorSpringMotion _expandedLabelMotion: MD.ColorSpringMotion {
        enabled: control._motionEnabled && m_label.visible
        targetColor: control.mdState.expandedLabelColor
        spec: control.effectsSpringSpec
    }
    readonly property bool hasIcon: !icon.empty
    readonly property bool _showIcon: iconStyle !== MD.Enum.TextOnly && hasIcon
    readonly property bool _showLabel: iconStyle !== MD.Enum.IconOnly
    readonly property bool _collapsedLabel: collapsedIconStyle !== MD.Enum.IconOnly
    readonly property real _progress: expansionProgress >= 0 ? MD.Util.clamp(expansionProgress, 0, 1) : _localProgress
    property real _localProgress: expand ? 1 : 0
    property bool _ready: false
    Component.onCompleted: _ready = true

    Behavior on _localProgress {
        enabled: control._ready && control.expansionProgress < 0
        NumberAnimation {
            duration: MD.Token.duration.long2
            easing: MD.Token.easing.emphasized
        }
    }

    property MD.StateRailItem mdState: MD.StateRailItem {
        item: control
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    padding: 0
    spacing: 0
    hoverEnabled: true

    icon.width: 24
    icon.height: 24
    icon.color: control.mdState.supportTextColor
    icon.fill: control.selected

    readonly property real _indicatorMargin: 16
    readonly property real _collapsedWidth: 96
    readonly property real _collapsedIndicatorW: 56
    readonly property real _collapsedIndicatorH: 32
    readonly property real _expandedIndicatorH: 56
    readonly property real _expandedLeadingPad: 16
    readonly property real _expandedTrailingPad: 24
    readonly property real _iconLabelSpacing: 12
    readonly property real _expandedLabelX: _expandedLeadingPad + _indicatorMargin + (_showIcon ? icon.width + _iconLabelSpacing : 0)
    readonly property real _expandedIndicatorW: {
        if (control.iconStyle === MD.Enum.IconOnly)
            return _expandedIndicatorH;
        return _expandedLeadingPad + _expandedTrailingPad + (_showIcon ? icon.width : 0) + (_showLabel ? m_label.implicitWidth : 0) + (_showIcon && _showLabel ? _iconLabelSpacing : 0);
    }
    readonly property real _expandedWidth: iconStyle === MD.Enum.IconOnly ? _expandedIndicatorW + _indicatorMargin * 2 : Math.max(_expandedIndicatorW, 220)
    readonly property real _implicitWidth: _collapsedWidth + (_expandedWidth - _collapsedWidth) * _progress
    readonly property real _collapsedHeight: _collapsedIndicatorH + (_collapsedLabel ? 4 + m_collapsed_label.implicitHeight : 0)
    readonly property real _itemHeight: _collapsedHeight + (_expandedIndicatorH - _collapsedHeight) * _progress

    function _visualX(leading, itemWidth) {
        return mirrored ? width - leading - itemWidth : leading;
    }

    MD.ToolTip.text: control.text
    MD.ToolTip.visible: (control._progress === 0 ? !control._collapsedLabel : !control._showLabel) && control.mdState.interactions.hovered && !control.mdState.interactions.pressed && MD.ToolTip.text.length > 0

    contentItem: Item {
        implicitWidth: control._implicitWidth
        implicitHeight: control._itemHeight

        MD.IconView {
            id: m_icon
            objectName: 'railItemIcon'
            visible: control._showIcon
            icon: control.icon
            x: control._visualX((control._collapsedWidth - width) / 2 + (control._expandedLeadingPad + control._indicatorMargin - (control._collapsedWidth - width) / 2) * control._progress, width)
            y: (control._collapsedIndicatorH - height) / 2 + (control._expandedIndicatorH - control._collapsedIndicatorH) / 2 * control._progress
        }
        MD.Text {
            id: m_collapsed_label
            visible: control._collapsedLabel && opacity > 0
            opacity: Math.max(0, 1 - control._progress * 3)
            text: control.text
            font.capitalization: Font.MixedCase
            typescale: MD.Token.typescale.label_medium
            color: control._collapsedLabelMotion.color
            x: control._visualX((control._collapsedWidth - implicitWidth) / 2, width)
            y: control._collapsedIndicatorH + 4
        }
        MD.Text {
            id: m_label
            visible: control._showLabel && opacity > 0
            opacity: MD.Util.clamp((control._progress - 0.2) / 0.8, 0, 1)
            text: control.text
            font.capitalization: Font.MixedCase
            typescale: MD.Token.typescale.label_large
            color: control._expandedLabelMotion.color
            x: control._visualX(control._expandedLabelX, width)
            y: (control._expandedIndicatorH - implicitHeight) / 2
            width: Math.max(0, parent.width - control._expandedLabelX - control._expandedTrailingPad)
            horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
        }
    }
    background: Item {
        implicitWidth: control._implicitWidth
        implicitHeight: control._itemHeight

        Item {
            id: m_indicator
            objectName: 'railIndicatorBounds'
            x: control._visualX((control._collapsedWidth - control._collapsedIndicatorW) / 2 + (control._indicatorMargin - (control._collapsedWidth - control._collapsedIndicatorW) / 2) * control._progress, width)
            y: 0
            width: control._collapsedIndicatorW + ((control.iconStyle === MD.Enum.IconOnly ? control._expandedIndicatorW : Math.max(0, control.width - control._indicatorMargin * 2)) - control._collapsedIndicatorW) * control._progress
            height: control._collapsedIndicatorH + (control._expandedIndicatorH - control._collapsedIndicatorH) * control._progress
            MD.Rectangle {
                objectName: 'railSelectionIndicator'
                width: parent.width * Math.max(0, control._selectionMotion.value)
                height: parent.height
                x: control._showLabel ? (control.mirrored ? parent.width - width : 0) : (parent.width - width) / 2
                radius: height / 2
                opacity: MD.Util.clamp(control._selectionMotion.value, 0, 1)
                color: control.mdState.selectedIndicatorColor
            }

            MD.Ripple {
                objectName: 'railRipple'
                anchors.fill: parent
                radius: height / 2
                interactionSource: control.interactionSource
                indication: control.mdState.indication
                color: control.mdState.stateLayerColor
            }
        }
    }
}
