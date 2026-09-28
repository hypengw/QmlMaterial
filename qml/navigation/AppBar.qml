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
    readonly property real collapsedFraction: expandedHeight > collapsedHeight ? Math.min(1, Math.max(0, -__offset / (expandedHeight - collapsedHeight))) : 0
    property real __offset: 0
    property bool __complete: false
    function __syncOffset(animate = true) {
        m_snap.stop();
        const target = scrollBehavior && scrollBehavior.enabled ? scrollBehavior.heightOffset : 0;
        if (__complete && animate && animationsEnabled && scrollBehavior && !scrollBehavior.active) {
            m_snap.from = __offset;
            m_snap.to = target;
            m_snap.start();
        } else {
            __offset = target;
        }
    }
    onScrollBehaviorChanged: __syncOffset(false)
    onExpandedHeightChanged: __syncOffset(false)
    onCollapsedHeightChanged: __syncOffset(false)
    onAnimationsEnabledChanged: __syncOffset(false)
    Component.onCompleted: {
        __complete = true;
        __syncOffset(false);
    }
    NumberAnimation {
        id: m_snap
        target: control
        property: "__offset"
        duration: MD.Token.duration.short4
        easing: MD.Token.easing.emphasized
    }
    Connections {
        target: control.scrollBehavior
        function onHeightOffsetChanged() {
            control.__syncOffset();
        }
        function onCollapseDistanceChanged() {
            control.__syncOffset(false);
        }
        function onEnabledChanged() {
            control.__syncOffset(false);
        }
        function onModeChanged() {
            control.__syncOffset(false);
        }
        function onInputStarted() {
            m_snap.stop();
            control.scrollBehavior.heightOffset = control.__offset;
        }
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
            y: control.scrollBehavior && control.type >= MD.Enum.AppBarMedium ? 64 : 0
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
                    m_title.y: m_title.parent.height - m_title.height - (control.type === MD.Enum.AppBarMedium ? 24 : 28)
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
            y: (64 - height) / 2
            width: m_text_row.width
        }

        RowLayout {
            id: m_bar_row
            y: (64 - height) / 2.0
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
        color: control.scrollBehavior && control.scrollBehavior.overlapped ? control.scrolledBackgroundColor : control.backgroundColor
        opacity: control.mdState.backgroundOpacity
        Behavior on color {
            enabled: control.__complete && control.animationsEnabled
            ColorAnimation {
                duration: MD.Token.duration.short4
                easing: MD.Token.easing.standard
            }
        }
    }
}
