import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.ToolBarBase {
    id: control
    property int radius: 0
    property int type: MD.Enum.AppBarCenterAligned
    property bool showBackground: true
    property list<MD.Action> actions
    property alias leadingAction: m_leading.action
    property alias title: m_title.text
    property MD.AppBarScroll scrollBehavior: null
    property real expandedHeight: mdState.containerHeight
    property real collapsedHeight: type >= MD.Enum.AppBarMedium ? 64 : 0
    property bool animationsEnabled: true
    property color backgroundColor: scrollBehavior ? MD.MProp.color.surface : mdState.backgroundColor
    property color scrolledBackgroundColor: MD.MProp.color.surface_container
    readonly property real collapsedFraction: scrollBehavior && scrollBehavior.enabled ? scrollBehavior.collapsedFraction : 0
    readonly property real __offset: scrollBehavior && scrollBehavior.enabled ? scrollBehavior.heightOffset : 0
    property bool __complete: false
    Component.onCompleted: {
        __complete = true;
    }
    Binding {
        target: control.scrollBehavior
        property: "animationsEnabled"
        value: control.animationsEnabled
        when: control.scrollBehavior !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
    property MD.StateAppBar mdState: MD.StateAppBar {
        item: control
    }
    Binding {
        control.mdState.type: control.type
        control.mdState.showBackground: control.showBackground
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: scrollBehavior ? Math.max(0, expandedHeight + __offset) : Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    leftPadding: 16 - (m_leading.visible ? m_leading.leftInset + m_leading.leftPadding : 0)
    rightPadding: leftPadding

    contentItem: Item {
        id: m_content
        implicitWidth: m_title.implicitWidth
        implicitHeight: m_title.implicitHeight
        clip: control.scrollBehavior !== null

        Item {
            y: control.scrollBehavior && control.type >= MD.Enum.AppBarMedium ? control.collapsedHeight : 0
            width: parent.width
            height: Math.max(0, parent.height - y)
            clip: control.scrollBehavior !== null && control.type >= MD.Enum.AppBarMedium
            MD.Text {
                id: m_title
                font.capitalization: Font.MixedCase
                typescale: control.mdState.typescale
                elide: Text.ElideRight
                wrapMode: Text.NoWrap
                opacity: control.type >= MD.Enum.AppBarMedium ? 1 - control.collapsedFraction : 1
                Binding {
                    when: control.type === MD.Enum.AppBarCenterAligned
                    restoreMode: Binding.RestoreNone
                    m_text_row.implicitWidth: 48 * 2.0 + m_title.implicitWidth / 2.0
                    m_title.x: m_bar_row.x + (m_bar_row.width - m_title.width) / 2.0
                    m_title.y: m_bar_row.y + (m_bar_row.height - m_title.height) / 2.0
                    m_title.horizontalAlignment: Text.AlignHCenter
                    m_title.width: m_text_row.width
                }
                Binding {
                    when: control.type === MD.Enum.AppBarSmall
                    restoreMode: Binding.RestoreNone
                    m_text_row.implicitWidth: m_title.implicitWidth
                    m_title.x: m_bar_row.x + m_text_row.x
                    m_title.y: m_bar_row.y + (m_bar_row.height - m_title.height) / 2.0
                    m_title.horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
                    m_title.width: m_text_row.width
                }
                Binding {
                    when: control.type >= MD.Enum.AppBarMedium
                    restoreMode: Binding.RestoreNone
                    m_text_row.implicitWidth: 0
                    m_title.x: 16 - control.leftPadding
                    m_title.y: {
                        const padding = control.type === MD.Enum.AppBarMedium ? 24 : 28;
                        const bottomPadding = control.scrollBehavior ? Math.max(0, Math.min(padding - (m_title.height - m_title.baselineOffset), Math.max(0, control.expandedHeight - control.collapsedHeight) - m_title.height)) : padding;
                        return m_title.parent.height - m_title.height - bottomPadding;
                    }
                    m_title.horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
                    m_title.width: m_title.parent.width - 16 * 2
                }
            }
        }

        MD.Text {
            text: m_title.text
            visible: control.scrollBehavior !== null && control.type >= MD.Enum.AppBarMedium
            opacity: MD.Util.bezierY(control.collapsedFraction, 0.8, 0, 0.8, 0.15)
            typescale: MD.Token.typescale.title_large
            font.capitalization: Font.MixedCase
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
            horizontalAlignment: control.mirrored ? Text.AlignRight : Text.AlignLeft
            x: m_bar_row.x + m_text_row.x
            y: (control.collapsedHeight - height) / 2
            width: m_text_row.width
        }

        RowLayout {
            id: m_bar_row
            y: ((control.scrollBehavior && control.type >= MD.Enum.AppBarMedium ? control.collapsedHeight : 64) - height) / 2.0
            width: parent.width
            layoutDirection: control.layoutDirection

            MD.IconButton {
                id: m_leading
                visible: action
            }

            Item {
                id: m_text_row
                Layout.fillWidth: true
            }

            MD.ActionToolBar {
                Layout.preferredWidth: 48 * 2
                Layout.fillWidth: true
                Layout.maximumWidth: maximumContentWidth + 2
                actions: control.actions
            }
        }
    }

    background: MD.Rectangle {
        //elevation: control.mdState.elevation
        //elevationItem.height: height - control.radius
        //elevationItem.y: control.radius
        //elevationItem.corners: MD.Util.corners(0)

        implicitHeight: control.mdState.containerHeight
        corners: MD.Util.corners(control.radius, 0)
        property real __overlapFraction: control.scrollBehavior && control.scrollBehavior.overlappedFraction > 0.01 ? 1 : 0
        color: control.scrollBehavior ? MD.Util.mixColorOklab(control.backgroundColor, control.scrolledBackgroundColor, control.type >= MD.Enum.AppBarMedium ? MD.Util.bezierY(control.collapsedFraction, 0.4, 0, 1, 1) : __overlapFraction) : control.backgroundColor
        opacity: control.mdState.backgroundOpacity
        Behavior on __overlapFraction {
            enabled: control.__complete && control.animationsEnabled && control.scrollBehavior !== null && control.type < MD.Enum.AppBarMedium
            NumberAnimation {
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.standard
            }
        }
        Behavior on color {
            enabled: control.__complete && control.animationsEnabled && control.scrollBehavior === null
            ColorAnimation {
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.standard
            }
        }
    }
}
