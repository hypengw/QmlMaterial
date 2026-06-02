pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Window

import Qcm.Material as MD

// M3 expressive wide navigation rail. Collapsed (96dp, vertical items) and
// expanded (220-360dp, horizontal items) share the same RailItem and transition
// into each other. When embedded it occupies layout space; otherwise the
// expanded rail is shown as a modal/modeless overlay (ModalWideNavigationRail).
T.Control {
    id: control
    focusPolicy: Qt.NoFocus
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    // -- data --
    property int currentIndex: 0
    property var model: null
    signal clicked(var model)

    // -- form state --
    // embedded (occupies space) when window is large enough
    readonly property bool useEmbed: MD.MProp.size.windowClass >= MD.Enum.WindowClassLarge
    // force the expanded rail to overlay even when it could embed
    property bool forceModal: false
    readonly property bool useModal: forceModal || !useEmbed
    // user-togglable expanded state for embedded mode
    property bool expanded: false
    // automatically expand when the window becomes large enough to embed
    property bool autoExpand: true
    // items use the horizontal (expanded) layout
    readonly property bool useLarge: (expanded && !useModal) || drawerOpened
    readonly property bool drawerOpened: m_private.drawerOpenedSet

    // -- sizes --
    property real collapsedWidth: 96
    // M3 wide rail expanded default = 220dp (narrower than the old drawer)
    property real expandedWidth: 220

    // -- arrangement --
    property int arrangement: MD.Enum.RailCenter

    // hide the collapsed rail completely on compact windows (no footprint)
    property bool hideWhenCollapsed: false

    // -- content slots --
    property Component header: defaultHeader
    property Component footer: null
    // custom FAB component; if unset, fabAction renders the built-in morphing FAB
    property Component fab: null
    // drives the built-in FAB: icon only when collapsed, extended icon+label when expanded
    property MD.Action fabAction: null
    readonly property Component defaultHeader: m_header_comp

    property MD.Action headerAction: defaultHeaderAction
    readonly property MD.Action defaultHeaderAction: MD.Action {
        icon.name: MD.Token.icon.menu
        text: ''
        onTriggered: control.toggle()
    }

    QtObject {
        id: m_private
        readonly property bool drawerOpened: m_drawer.position > control.collapsedWidth / Math.max(m_drawer.implicitWidth, 1)
        property bool drawerOpenedSet: false
    }

    function open() {
        if (useModal)
            m_drawer.open();
        else
            expanded = true;
    }

    function close() {
        if (useModal)
            m_drawer.close();
        else
            expanded = false;
    }

    function toggle() {
        if (useModal) {
            if (m_drawer.visible)
                m_drawer.close();
            else
                m_drawer.open();
        } else {
            expanded = !expanded;
        }
    }

    onUseEmbedChanged: {
        if (useEmbed)
            m_drawer.close();
        if (autoExpand)
            expanded = useEmbed;
    }
    Component.onCompleted: {
        if (autoExpand && useEmbed)
            expanded = true;
    }

    background: Item {
        implicitWidth: {
            if (control.hideWhenCollapsed && MD.MProp.size.isCompact)
                return 0;
            if (control.useModal)
                return control.collapsedWidth;
            return control.expanded ? control.expandedWidth : control.collapsedWidth;
        }
        implicitHeight: 400

        Behavior on implicitWidth {
            NumberAnimation {
                duration: MD.Token.duration.long2
                easing: MD.Token.easing.emphasized
            }
        }
    }

    contentItem: Item {
        id: m_embed_content
    }

    Component {
        id: m_header_comp
        Item {
            implicitWidth: control.useLarge ? Math.max(m_fab.x + m_fab.width + 12, 220) : control.collapsedWidth
            implicitHeight: m_menu_btn.y + m_menu_btn.height + (m_fab.visible ? m_fab.height + 12 : 0) + 12

            MD.StandardIconButton {
                id: m_menu_btn
                // expanded x aligns the menu icon left edge (btn is 40 wide, icon 24 centered)
                // with the nav item icons at x=32; collapsed target keeps it centered in the rail
                x: control.useLarge ? (32 - (width - 24) / 2) : (control.collapsedWidth - width) / 2
                y: 4
                icon.name: control.useLarge ? MD.Token.icon.menu_open : MD.Token.icon.menu
                onClicked: control.headerAction ? control.headerAction.trigger() : control.toggle()

                Behavior on x {
                    NumberAnimation {
                        duration: MD.Token.duration.long2
                        easing: MD.Token.easing.emphasized
                    }
                }
            }

            // built-in morphing FAB (icon-only -> extended)
            MD.ElevationRectangle {
                id: m_fab
                visible: !control.fab && control.fabAction
                // expanded x=16 aligns the FAB indicator left edge with the nav item indicators;
                // collapsed target uses the final collapsed width (56) so x and width animate together
                x: control.useLarge ? 16 : (control.collapsedWidth - 56) / 2
                y: m_menu_btn.y + m_menu_btn.height + 12
                height: 56
                width: control.useLarge ? (16 + m_fab_icon.width + 12 + m_fab_label.implicitWidth + 20) : 56
                radius: MD.Token.shape.corner.large
                color: MD.MProp.color.primary_container
                elevationVisible: false

                Behavior on x {
                    NumberAnimation {
                        duration: MD.Token.duration.long2
                        easing: MD.Token.easing.emphasized
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: MD.Token.duration.long2
                        easing: MD.Token.easing.emphasized
                    }
                }

                MD.Icon {
                    id: m_fab_icon
                    name: control.fabAction?.icon.name ?? ''
                    size: 24
                    color: MD.MProp.color.on_primary_container
                    x: control.useLarge ? 16 : (56 - width) / 2
                    y: (parent.height - height) / 2
                }
                MD.Text {
                    id: m_fab_label
                    text: control.fabAction?.text ?? ''
                    visible: control.useLarge
                    typescale: MD.Token.typescale.label_large
                    color: MD.MProp.color.on_primary_container
                    x: m_fab_icon.x + m_fab_icon.width + 12
                    y: (parent.height - height) / 2
                }
                MD.Ripple {
                    anchors.fill: parent
                    radius: parent.radius
                    color: MD.MProp.color.on_primary_container
                    pressed: m_fab_tap.pressed
                    pressX: m_fab_tap.point.position.x
                    pressY: m_fab_tap.point.position.y
                    stateOpacity: m_fab_tap.pressed ? MD.Token.state.pressed.state_layer_opacity : 0
                }
                TapHandler {
                    id: m_fab_tap
                    onTapped: control.fabAction?.trigger()
                }
            }

            // custom FAB slot
            Loader {
                id: m_fab_loader
                x: control.useLarge ? 12 : (control.collapsedWidth - width) / 2
                y: m_menu_btn.y + m_menu_btn.height + 12
                visible: sourceComponent
                sourceComponent: control.fab
            }
        }
    }

    MD.Drawer {
        id: m_drawer
        parent: T.Overlay.overlay
        modal: control.useModal
        MD.MProp.textColor: MD.MProp.color.on_surface
        MD.MProp.backgroundColor: MD.MProp.color.surface_container

        topPadding: 4
        bottomPadding: 4

        background: Item {
            implicitWidth: control.expandedWidth
            Rectangle {
                anchors.fill: parent
                color: MD.MProp.color.surface_container
                layer.enabled: true
                layer.effect: MD.RoundClip {
                    corners: MD.Util.corners(0, 16, 0, 16)
                    size: Qt.vector2d(parent.width, parent.height)
                }
            }
        }

        contentItem: Item {
            id: m_popup_content
            implicitWidth: m_flick.implicitWidth
            implicitHeight: m_flick.implicitHeight
        }
    }

    Item {
        visible: false
        state: m_private.drawerOpened ? "popup" : "embed"

        states: [
            State {
                name: "embed"
                ParentChange {
                    target: m_flick
                    parent: m_embed_content
                }
                PropertyChanges {
                    m_flick.visible: true
                    m_private.drawerOpenedSet: false
                }
            },
            State {
                name: "popup"
                ParentChange {
                    target: m_flick
                    parent: m_popup_content
                }
                PropertyChanges {
                    m_flick.visible: true
                    m_private.drawerOpenedSet: true
                }
            }
        ]

        MD.VerticalFlickable {
            id: m_flick
            topMargin: 12
            bottomMargin: 12
            implicitWidth: m_content.implicitWidth
            implicitHeight: m_content.implicitHeight + 12 * 2
            anchors.fill: parent

            opacity: {
                const v = m_drawer.position;
                const left = control.collapsedWidth / Math.max(m_drawer.implicitWidth, 1);
                const right = left + 0.1;
                return control.useModal ? MD.Util.teleportCurve(v, left, right) : 1;
            }

            Item {
                id: m_content
                width: parent.width
                height: Math.max(implicitHeight, m_flick.height - 12 * 2)
                implicitWidth: control.useLarge ? control.expandedWidth : control.collapsedWidth
                implicitHeight: m_header_loader.height + m_rail_container.implicitHeight + m_footer_container.implicitHeight

                // -- header --
                Loader {
                    id: m_header_loader
                    width: parent.width
                    y: 0
                    visible: sourceComponent
                    sourceComponent: control.header
                }

                // -- rail items --
                Item {
                    id: m_rail_container
                    y: m_header_loader.y + m_header_loader.height
                    width: parent.width
                    height: Math.max(implicitHeight, m_content.height - m_header_loader.height - m_footer_container.implicitHeight)
                    implicitHeight: m_rail_view.implicitHeight

                    MD.VerticalListView {
                        id: m_rail_view
                        expand: true
                        y: {
                            if (control.useLarge)
                                return 0;
                            const max = Math.max(parent.height - height, 0);
                            switch (control.arrangement) {
                            case MD.Enum.RailCenter:
                                // center within the whole control, not just the
                                // space left between header and footer
                                return MD.Util.clamp((m_content.height - height) / 2 - m_header_loader.height, 0, max);
                            case MD.Enum.RailBottom:
                                return max;
                            default:
                                return 0;
                            }
                        }
                        height: implicitHeight
                        width: parent.width

                        spacing: control.useLarge ? 0 : 6

                        interactive: false
                        reuseItems: false
                        currentIndex: control.currentIndex
                        model: control.model
                        // a model entry with a `section` field renders as a group
                        // header (text when expanded, a short divider when collapsed)
                        delegate: Item {
                            id: m_delegate
                            required property var model
                            required property int index
                            readonly property bool isHeader: m_delegate.model.section !== undefined
                            width: ListView.view.contentWidth
                            implicitHeight: isHeader ? m_section.implicitHeight : m_nav.implicitHeight
                            height: implicitHeight

                            Item {
                                id: m_section
                                visible: m_delegate.isHeader
                                width: parent.width
                                implicitHeight: control.useLarge ? (12 + m_section_label.implicitHeight + 8) : 21

                                MD.Text {
                                    id: m_section_label
                                    visible: control.useLarge
                                    x: 16
                                    y: 12
                                    width: parent.width - 32
                                    text: m_delegate.model.section ?? ''
                                    typescale: MD.Token.typescale.title_small
                                    color: MD.MProp.color.on_surface_variant
                                }
                                Rectangle {
                                    visible: !control.useLarge
                                    width: 48
                                    height: 1
                                    x: (parent.width - width) / 2
                                    y: (parent.implicitHeight - height) / 2
                                    color: MD.MProp.color.outline_variant
                                }
                            }

                            MD.RailItem {
                                id: m_nav
                                visible: !m_delegate.isHeader
                                width: parent.width
                                icon.name: {
                                    const ic = m_delegate.model.icon;
                                    if (typeof ic == 'number')
                                        return MD.Token.icon[ic];
                                    return ic ?? '';
                                }
                                text: m_delegate.model.name ?? ''
                                expand: control.useLarge
                                checked: control.currentIndex === m_delegate.index
                                onClicked: {
                                    control.currentIndex = m_delegate.index;
                                    control.clicked(m_delegate.model);
                                    if (control.useModal)
                                        m_drawer.close();
                                }
                            }
                        }
                    }
                }

                // -- footer --
                Item {
                    id: m_footer_container
                    y: m_rail_container.y + m_rail_container.height
                    width: parent.width
                    height: implicitHeight
                    implicitHeight: m_footer_loader.sourceComponent ? m_footer_loader.implicitHeight : 0

                    Loader {
                        id: m_footer_loader
                        asynchronous: true
                        width: parent.width
                        sourceComponent: control.footer
                    }
                }
            }
        }
    }
}
