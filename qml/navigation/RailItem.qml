import QtQuick
import Qcm.Material as MD

MD.ButtonBase {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    property int collapsedIconStyle: iconStyle
    property bool expand: false
    property real expansionProgress: -1
    property Item trailing: null
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
    flat: false
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
    icon.fill: control.checked

    readonly property real _indicatorMargin: 16
    readonly property real _collapsedWidth: 96
    readonly property real _collapsedIndicatorW: 56
    readonly property real _collapsedIndicatorH: 32
    readonly property real _expandedIndicatorH: 56
    readonly property real _expandedLeadingPad: 16
    readonly property real _expandedTrailingPad: 24
    readonly property real _iconLabelSpacing: 12
    readonly property real _expandedIndicatorW: {
        if (control.iconStyle === MD.Enum.IconOnly)
            return _expandedIndicatorH;
        return _expandedLeadingPad + _expandedTrailingPad + (_showIcon ? icon.width : 0) + (_showLabel ? m_label.implicitWidth : 0) + (_showIcon && _showLabel ? _iconLabelSpacing : 0);
    }
    readonly property real _expandedWidth: iconStyle === MD.Enum.IconOnly ? _expandedIndicatorW + _indicatorMargin * 2 : Math.max(_expandedIndicatorW, 220)
    readonly property real _implicitWidth: _collapsedWidth + (_expandedWidth - _collapsedWidth) * _progress
    readonly property real _collapsedHeight: _collapsedIndicatorH + (_collapsedLabel ? 4 + m_collapsed_label.implicitHeight : 0)
    readonly property real _itemHeight: _collapsedHeight + (_expandedIndicatorH - _collapsedHeight) * _progress

    MD.ToolTip.text: control.action?.tooltip || control.text || ""
    MD.ToolTip.visible: (control._progress === 0 ? !control._collapsedLabel : !control._showLabel) && control.hovered && !control.pressed && MD.ToolTip.text.length > 0

    contentItem: Item {
        implicitWidth: control._implicitWidth
        implicitHeight: control._itemHeight

        MD.IconView {
            id: m_icon
            visible: control._showIcon
            icon: control.icon
            x: (control._collapsedWidth - width) / 2 + (control._expandedLeadingPad + control._indicatorMargin - (control._collapsedWidth - width) / 2) * control._progress
            y: (control._collapsedIndicatorH - height) / 2 + (control._expandedIndicatorH - control._collapsedIndicatorH) / 2 * control._progress
        }
        MD.Text {
            id: m_collapsed_label
            visible: control._collapsedLabel && opacity > 0
            opacity: Math.max(0, 1 - control._progress * 3)
            text: control.text
            font.capitalization: Font.MixedCase
            typescale: MD.Token.typescale.label_medium
            prominent: control.checked
            color: control.mdState.collapsedLabelColor
            x: (control._collapsedWidth - implicitWidth) / 2
            y: control._collapsedIndicatorH + 4
        }
        MD.Text {
            id: m_label
            visible: control._showLabel && opacity > 0
            opacity: MD.Util.clamp((control._progress - 0.2) / 0.8, 0, 1)
            text: control.text
            font.capitalization: Font.MixedCase
            typescale: MD.Token.typescale.label_large
            prominent: control.checked
            color: control.mdState.expandedLabelColor
            x: control._expandedLeadingPad + control._indicatorMargin + (control._showIcon ? control.icon.width + control._iconLabelSpacing : 0)
            y: (control._expandedIndicatorH - implicitHeight) / 2
            width: Math.max(0, parent.width - x - control._expandedTrailingPad)
            elide: Text.ElideRight
        }
    }
    background: Item {
        implicitWidth: control._implicitWidth
        implicitHeight: control._itemHeight

        MD.ElevationRectangle {
            id: m_indicator
            x: (control._collapsedWidth - control._collapsedIndicatorW) / 2 + (control._indicatorMargin - (control._collapsedWidth - control._collapsedIndicatorW) / 2) * control._progress
            y: 0
            width: control._collapsedIndicatorW + ((control.iconStyle === MD.Enum.IconOnly ? control._expandedIndicatorW : Math.max(0, control.width - control._indicatorMargin * 2)) - control._collapsedIndicatorW) * control._progress
            height: control._collapsedIndicatorH + (control._expandedIndicatorH - control._collapsedIndicatorH) * control._progress
            radius: height / 2
            color: control.mdState.backgroundColor
            elevationVisible: control.enabled && color.a > 0
            elevation: control.mdState.elevation

            MD.Ripple {
                anchors.fill: parent
                readonly property point p: control.mapToItem(this, control.pressX, control.pressY)
                radius: parent.radius
                pressX: p.x
                pressY: p.y
                pressed: control.pressed
                stateOpacity: control.mdState.stateLayerOpacity
                color: control.mdState.stateLayerColor
            }
        }
    }
}
