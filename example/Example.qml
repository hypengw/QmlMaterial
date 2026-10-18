import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

MD.Page {
    id: root
    property int pageIndex: 0
    padding: 8
    leftPadding: 0

    readonly property bool isCompact: MD.MProp.size.isCompact

    readonly property var pageModel: [
        {
            name: 'components',
            source: 'qrc:/Qcm/Material/Example/Components.qml',
            icon: MD.Token.icon.widgets
        },
        {
            name: 'compositions',
            source: 'qrc:/Qcm/Material/Example/Compositions.qml',
            icon: MD.Token.icon.dashboard
        },
        {
            name: 'color',
            source: 'qrc:/Qcm/Material/Example/Color.qml',
            icon: MD.Token.icon.palette
        },
        {
            name: 'shape',
            source: 'qrc:/Qcm/Material/Example/Shape.qml',
            icon: MD.Token.icon.shapes
        }
    ]

    readonly property var popupModel: [
        {
            name: 'typography',
            source: 'qrc:/Qcm/Material/Example/Typography.qml',
            icon: MD.Token.icon.description
        },
        {
            name: 'about',
            source: 'qrc:/Qcm/Material/Example/About.qml',
            icon: MD.Token.icon.info
        }
    ]

    function openUtilityPopup(source) {
        utilityPresenter.present(source);
    }

    MD.PopupPresenter {
        id: utilityPresenter
        objectName: 'utilityPopupPresenter'
        host: root
    }

    onPageIndexChanged: {
        m_container.switchTo(pageModel[pageIndex].source, {}, false);
    }

    Component.onCompleted: {
        pageIndexChanged();
        MD.Token.color.useSysColorSM = false;
    }

    MD.MProp.page: m_page_ctx

    MD.PageContext {
        id: m_page_ctx
        showHeader: false
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Loader {
                id: m_drawer_loader
                Layout.fillHeight: true
                active: !root.isCompact
                visible: active

                sourceComponent: MD.NavigationRail {
                    id: m_rail
                    objectName: 'exampleNavigationRail'
                    autoExpand: false
                    model: root.pageModel
                    currentIndex: root.pageIndex

                    fabAction: MD.Action {
                        icon.name: MD.Token.icon.add
                        text: 'Create'
                    }

                    onClicked: function (model) {
                        root.pageIndex = model.index;
                    }

                    // theme panel only fits the expanded rail
                    footer: Item {
                        implicitHeight: m_rail.useLarge ? popupLinks.implicitHeight + m_panel.implicitHeight + 24 : 0

                        Column {
                            id: popupLinks
                            width: parent.width
                            visible: m_rail.useLarge
                            Repeater {
                                model: root.popupModel
                                MD.RailItem {
                                    required property var modelData
                                    objectName: modelData.name + 'PopupButton'
                                    width: popupLinks.width
                                    expand: m_rail.useLarge
                                    expansionProgress: m_rail.expansionProgress
                                    collapsedIconStyle: MD.Enum.IconOnly
                                    text: modelData.name
                                    icon.name: modelData.icon
                                    onClicked: {
                                        if (m_rail.useModal)
                                            m_rail.close();
                                        root.openUtilityPopup(modelData.source);
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            id: m_panel
                            visible: m_rail.useLarge
                            x: 16
                            y: popupLinks.height + 8
                            width: parent.width - 32
                            spacing: 8
                            RowLayout {
                                MD.Label {
                                    text: 'Brightness'
                                    typescale: MD.Token.typescale.label_large
                                }
                                Item {
                                    Layout.fillWidth: true
                                }

                                MD.Switch {
                                    checked: !MD.Token.isDarkTheme
                                    onToggled: {
                                        const tk = MD.Token;
                                        if (tk.isDarkTheme) {
                                            tk.themeMode = MD.Enum.Light;
                                        } else {
                                            tk.themeMode = MD.Enum.Dark;
                                        }
                                    }
                                }
                            }

                            MD.HorizontalListView {
                                id: m_palette_view
                                Layout.fillWidth: true
                                expand: true
                                spacing: 8
                                implicitHeight: 40
                                model: MD.PaletteModel {}
                                Component.onCompleted: currentIndex = MD.Token.color.paletteType
                                MD.ActionGroup {
                                    id: m_palette_group
                                }
                                delegate: MD.InputChip {
                                    required property int index
                                    required property var model
                                    action: MD.Action {
                                        MD.ActionGroup.group: m_palette_group
                                        icon.name: ''
                                        checkable: true
                                        checked: m_palette_view.currentIndex === index
                                        onTriggered: function (c) {
                                            m_palette_view.currentIndex = index;
                                            MD.Token.color.paletteType = index;
                                        }
                                        text: model.name
                                    }
                                }
                            }

                            MD.AutoDivider {}

                            Grid {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 16
                                rows: 3
                                columns: 3
                                Repeater {
                                    model: [
                                        {
                                            name: 'Red',
                                            color: '#F44336'
                                        },
                                        {
                                            name: 'Pink',
                                            color: '#E91E63'
                                        },
                                        {
                                            name: 'Purple',
                                            color: '#9C27B0'
                                        },
                                        {
                                            name: 'Indigo',
                                            color: '#3F51B5'
                                        },
                                        {
                                            name: 'Teal',
                                            color: '#009688'
                                        },
                                        {
                                            name: 'LightGreen',
                                            color: '#8BC34A'
                                        },
                                        {
                                            name: 'Yellow',
                                            color: '#FFEB3B'
                                        },
                                        {
                                            name: 'Amber',
                                            color: '#FFC107'
                                        },
                                        {
                                            name: 'Orange',
                                            color: '#FF9800'
                                        }
                                    ]
                                    MD.ColorRadio {
                                        size: 28
                                        color: modelData.color
                                        checked: MD.Token.color.accentColor === color
                                        onClicked: {
                                            MD.Token.color.accentColor = color;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            MD.PageContainer {
                id: m_container
                objectName: 'examplePages'
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                initialItem: Item {}
                MD.MProp.page: m_page_ctx_c

                MD.PageContext {
                    id: m_page_ctx_c
                    showHeader: false
                    backgroundRadius: 12
                    showBackground: true
                }
            }
        }

        // bottom navigation bar (compact mode)
        Loader {
            id: m_bar_loader
            Layout.fillWidth: true
            active: root.isCompact
            visible: active

            sourceComponent: MD.Pane {
                padding: 0
                backgroundColor: MD.MProp.color.surface_container

                contentItem: Item {
                    implicitHeight: 88

                    MD.Flickable {
                        id: navigation
                        objectName: 'compactNavigation'
                        anchors.fill: parent
                        anchors.rightMargin: moreButton.width
                        contentWidth: navigationItems.width
                        contentHeight: height
                        flickableDirection: Flickable.HorizontalFlick
                        function revealCurrent() {
                            const item = navigationRepeater.itemAt(root.pageIndex);
                            if (!item)
                                return;
                            const limit = Math.max(0, contentWidth - width);
                            let position = Math.min(contentX, item.x);
                            position = Math.max(position, item.x + item.width - width);
                            contentX = Math.max(0, Math.min(position, limit));
                        }
                        onWidthChanged: Qt.callLater(revealCurrent)
                        Row {
                            id: navigationItems
                            Repeater {
                                id: navigationRepeater
                                model: root.pageModel
                                Item {
                                    width: Math.max(104, navigation.width / root.pageModel.length)
                                    height: navigation.height
                                    required property var modelData
                                    required property int index
                                    MD.BarItem {
                                        objectName: 'compactNavigationButton' + parent.index
                                        anchors.fill: parent
                                        anchors.topMargin: 12
                                        anchors.bottomMargin: 16
                                        icon.name: parent.modelData.icon
                                        text: parent.modelData.name
                                        checked: root.pageIndex == parent.index
                                        onCheckedChanged: if (checked)
                                            Qt.callLater(navigation.revealCurrent)
                                        onClicked: {
                                            root.pageIndex = parent.index;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    MD.BarItem {
                        id: moreButton
                        objectName: 'compactMoreButton'
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.topMargin: 12
                        anchors.bottomMargin: 16
                        width: 88
                        text: 'More'
                        icon.name: MD.Token.icon.more_horiz
                        onClicked: utilityMenu.openFor(moreButton)

                        MD.Menu {
                            id: utilityMenu
                            objectName: 'compactUtilityMenu'
                            preferredPlacement: MD.PopupBase.Above
                            anchorAlignment: MD.PopupBase.End
                            model: root.popupModel
                            contentDelegate: MD.MenuItem {
                                required property var modelData
                                objectName: modelData.name + 'MenuItem'
                                text: modelData.name
                                icon.name: modelData.icon
                                onTriggered: root.openUtilityPopup(modelData.source)
                            }
                        }
                    }
                }
            }
        }
    }
}
