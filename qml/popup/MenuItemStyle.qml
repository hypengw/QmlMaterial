pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.Material.Layouts as Lite

MD.StateMenuItem {
    id: style
    readonly property var control: item
    readonly property MD.Menu owner: control ? (control.menu as MD.Menu) ?? control.ListView.view?.menu ?? null : null
    expressive: owner?.variant !== MD.Menu.Variant.Baseline
    vibrant: owner?.vibrant ?? false

    property string supportText
    property Component supporting: null
    property string trailingText
    property string trailingIcon
    property Component badge
    readonly property bool submenuActive: (control?.subMenu?.visible ?? false) && !control.subMenu.closing && control.subMenu.anchorItem === control
    readonly property bool showSelectionMark: expressive && selected && (control?.icon.empty ?? false)
    readonly property var groups: owner?.__groups ?? []
    readonly property int groupIndex: groups.findIndex(group => group.first === control || group.last === control)

    property Item arrow: MD.Icon {
        parent: content_layout

        visible: !!control?.subMenu
        size: expressive ? MD.MenuMetrics.iconSize : MD.MenuMetrics.baselineIconSize
        name: MD.Token.icon.arrow_right
        color: control.trailingIconColor
        rotation: control.mirrored ? 180 : 0
    }

    readonly property Item contentItem: Lite.Row {
        id: content_layout
        opacity: control.mdState.contentOpacity
        alignment: Qt.AlignVCenter
        spacing: control.spacing
        LayoutMirroring.enabled: control.mirrored

        Item {
            implicitWidth: Math.max(m_leading_icon.implicitWidth, m_leading_loader.implicitWidth, selectionMark.implicitWidth)
            implicitHeight: Math.max(m_leading_icon.implicitHeight, m_leading_loader.implicitHeight, selectionMark.implicitHeight)
            visible: !control.icon.empty || m_leading_loader.active || style.showSelectionMark

            MD.IconView {
                id: m_leading_icon
                anchors.centerIn: parent
                visible: !icon.empty && !m_leading_loader.active
                icon: control.icon
            }

            MD.Icon {
                id: selectionMark
                anchors.centerIn: parent
                visible: style.showSelectionMark && !m_leading_loader.active
                name: MD.Token.icon.check
                size: control.icon.width
                color: control.leadingIconColor
            }

            MD.Loader {
                id: m_leading_loader
                anchors.centerIn: parent
                active: control.busy !== MD.Enum.Idle
                visible: active
                sourceComponent: m_loading_comp
            }
        }

        Item {
            implicitWidth: Math.max(mainLabel.implicitWidth, style.supporting !== null ? supportingLoader.item?.implicitWidth ?? 0 : style.supportText.length > 0 ? supportingLabel.implicitWidth : 0)
            implicitHeight: mainLabel.implicitHeight + (style.supporting !== null ? supportingLoader.item?.implicitHeight ?? 0 : style.supportText.length > 0 ? supportingLabel.implicitHeight : 0)
            Lite.Layout.fillWidth: true

            MD.Label {
                id: mainLabel
                width: parent.width
                text: control.text
                color: control.mdState.textColor
                useTypescale: false
                font: control.font
                lineHeight: control.typescale.line_height
                horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
                wrapMode: Text.NoWrap
            }
            MD.Label {
                id: supportingLabel
                y: mainLabel.height
                width: parent.width
                visible: style.supporting === null && text.length > 0
                text: style.supportText
                color: control.mdState.supportTextColor
                typescale: MD.Token.typescale.body_small
                horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
                wrapMode: Text.NoWrap
            }

            MD.Loader {
                id: supportingLoader
                y: mainLabel.height
                width: parent.width
                active: style.supporting !== null
                visible: active
                sourceComponent: style.supporting
            }
        }

        MD.Loader {
            active: style.badge !== null
            visible: active
            sourceComponent: style.badge
        }

        MD.Label {
            visible: text.length > 0
            text: style.trailingText
            color: control.mdState.supportTextColor
            typescale: MD.Token.typescale.label_large
            wrapMode: Text.NoWrap
        }

        MD.Icon {
            visible: name.length > 0
            name: style.trailingIcon
            color: control.trailingIconColor
            size: control.icon.width
        }

        Component {
            id: m_loading_comp
            MD.CircularIndicator {
                anchors.centerIn: parent
                indeterminate: control.busy !== MD.Enum.Progress
                running: control.busy !== MD.Enum.Progress
                value: control.progress
                strokeWidth: 2
                implicitWidth: {
                    const w = Math.min(control.icon.width, control.icon.height);
                    return w - 6;
                }
                implicitHeight: implicitWidth
            }
        }
    }

    readonly property Item background: Item {
        implicitHeight: MD.MenuMetrics.itemHeight

        MD.Rectangle {
            id: shape
            anchors.fill: parent
            anchors.leftMargin: style.expressive ? MD.MenuMetrics.horizontalInset : 0
            anchors.rightMargin: anchors.leftMargin
            radius: !style.expressive ? 0 : style.selected || style.submenuActive ? MD.MenuMetrics.selectedRadius : MD.MenuMetrics.itemRadius
            readonly property var group: style.groups[style.groupIndex]
            readonly property real outerRadius: Math.max(0, (style.owner?.__outerRadius ?? MD.MenuMetrics.containerRadius) - MD.MenuMetrics.horizontalInset)
            readonly property real topRadius: group?.first === control && style.groupIndex === 0 ? Math.max(radius, outerRadius) : radius
            readonly property real bottomRadius: group?.last === control && style.groupIndex === style.groups.length - 1 ? Math.max(radius, outerRadius) : radius
            corners: MD.Util.corners(topRadius, bottomRadius)
            color: control.mdState.backgroundColor

            Behavior on radius {
                NumberAnimation {
                    duration: MD.Token.duration.short3
                    easing: MD.Token.easing.standard
                }
            }

            MD.Rectangle {
                anchors.fill: parent
                corners: shape.corners
                color: control.mdState.stateLayerColor
                opacity: control.enabled && (control.highlighted || style.submenuActive) && control.mdState.stateLayerOpacity === 0 ? MD.Token.state.hover.state_layer_opacity : 0
            }

            MD.Ripple {
                anchors.fill: parent
                corners: shape.corners
                interactionSource: control.interactionSource
                indication: control.mdState.indication
                color: control.mdState.stateLayerColor
            }

            MD.FocusIndicator {
                corners: shape.corners
                active: style.expressive && control.focusVisible
                inset: true
            }
        }
    }
}
