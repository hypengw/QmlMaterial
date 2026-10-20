pragma ComponentBehavior: Bound
import QtQuick

import Qcm.Material as MD

MD.StandardDrawerBase {
    id: control
    focusPolicy: Qt.NoFocus
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    // whether to show embedded sidebar (large window) vs popup-only
    useEmbed: MD.MProp.size.windowClass >= MD.Enum.WindowClassLarge
    // whether items are in expanded layout (icon+label horizontal)
    readonly property bool useLarge: expanded || drawerOpened

    // configurable sizes for collapsed/expanded states
    property real collapsedWidth: 96
    property real expandedWidth: 360

    property var model: null
    property Component header: defaultHeader
    property Component footer: null
    property Component drawerContent: null

    property Component drawerHeader: null
    readonly property Component defaultHeader: m_menu_comp

    readonly property bool drawerOpened: m_private.drawerOpenedSet

    property MD.Action headerAction: defaultHeaderAction
    defaultHeaderAction.icon.name: MD.Token.icon.menu
    drawer: m_drawer
    headerControl: m_header_loader.item as MD.CommandButtonBase

    QtObject {
        id: m_private
        readonly property bool drawerOpened: (m_drawer?.position ?? 0) > control.collapsedWidth / Math.max(1, m_drawer?.implicitWidth ?? control.expandedWidth)
        property bool drawerOpenedSet: false
    }

    background: Item {
        implicitWidth: {
            if (MD.MProp.size.windowClass >= MD.Enum.WindowClassLarge)
                return control.expanded ? control.expandedWidth : control.collapsedWidth;
            if (!MD.MProp.size.isCompact)
                return control.collapsedWidth;
            return 0;
        }
        implicitHeight: 400
    }

    contentItem: Item {
        id: m_embed_content
    }

    Component {
        id: m_menu_comp
        MD.StandardIconButton {
            visible: !control.useEmbed
            implicitWidth: control.collapsedWidth
            leftInset: Math.max(4, (width - implicitBackgroundSize) / 2)
            rightInset: leftInset
            MD.ActionCommand.action: control.headerAction ?? control.defaultHeaderAction
            text: MD.ActionCommand.text
            icon.actionIcon: MD.ActionCommand.icon
            enabled: MD.ActionCommand.enabled
            MD.ToolTip.text: text
            MD.ToolTip.visible: mdState.interactions.hovered && !mdState.interactions.pressed && text.length > 0
        }
    }

    MD.Drawer {
        id: m_drawer
        parent: control.MD.Overlay.overlay
        MD.MProp.textColor: MD.MProp.color.on_surface
        MD.MProp.backgroundColor: MD.MProp.color.surface_container

        topPadding: 4
        bottomPadding: 4
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
            enabled: control.enabled && control.visible
            topMargin: 12
            bottomMargin: 12
            implicitWidth: m_content.implicitWidth
            implicitHeight: m_content.implicitHeight + 12 * 2
            anchors.fill: parent

            opacity: {
                const v = m_drawer?.position ?? 0;
                const left = control.collapsedWidth / Math.max(1, m_drawer?.implicitWidth ?? control.expandedWidth);
                const right = left + 0.1;
                return MD.Util.teleportCurve(v, left, right);
            }

            Item {
                id: m_content
                width: parent.width
                height: Math.max(implicitHeight, m_flick.height - 12 * 2)
                implicitWidth: Math.min(control.expandedWidth, Math.max(m_rail_view.implicitWidth, m_footer_loader.implicitWidth))
                implicitHeight: m_header_loader.height + m_rail_container.implicitHeight + m_footer_container.implicitHeight

                // -- header --
                Loader {
                    id: m_header_loader
                    width: parent.width
                    y: 0
                    visible: sourceComponent
                    sourceComponent: control.useLarge ? control.drawerHeader : control.header
                }

                // -- rail items --
                Item {
                    id: m_rail_container
                    y: m_header_loader.y + m_header_loader.height
                    width: parent.width
                    height: control.useLarge ? implicitHeight : Math.max(implicitHeight, m_content.height - m_header_loader.height - m_footer_container.implicitHeight)
                    implicitHeight: m_rail_view.implicitHeight
                    implicitWidth: m_rail_view.implicitWidth

                    MD.VerticalListView {
                        id: m_rail_view
                        y: {
                            if (control.useLarge)
                                return 0;
                            const center = (parent.height - height) / 2;
                            const min = 0;
                            const max = Math.max(parent.height - height, 0);
                            return MD.Util.clamp(center, min, max);
                        }
                        implicitHeight: contentHeight
                        height: implicitHeight
                        width: parent.width

                        spacing: control.useLarge ? 0 : 12

                        interactive: false
                        reuseItems: false
                        currentIndex: control.currentIndex
                        model: control.model
                        delegate: MD.RailItem {
                            required property var model
                            required property int index
                            width: ListView.view.contentWidth
                            icon.name: {
                                if (typeof model.icon == 'number')
                                    return MD.Token.icon[model.icon];
                                else
                                    return model.icon;
                            }
                            text: model.name
                            expand: control.useLarge
                            selected: control.currentIndex == index
                            navigation: control
                            navigationIndex: index
                        }
                    }
                }

                // -- footer / drawer content --
                Item {
                    id: m_footer_container
                    y: m_rail_container.y + m_rail_container.height
                    width: parent.width
                    height: control.useLarge ? (m_content.height - y) : implicitHeight
                    implicitHeight: m_footer_loader.sourceComponent ? m_footer_loader.implicitHeight : 0

                    Loader {
                        id: m_footer_loader
                        asynchronous: true
                        height: parent.height
                        width: Math.max(parent.width, control.implicitBackgroundWidth)
                        sourceComponent: control.useLarge ? control.drawerContent : control.footer
                    }
                }
            }
        }
    }
}
