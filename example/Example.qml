import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Page {
    id: root
    property int pageIndex: 0
    padding: 8
    title: 'Material 3'

    readonly property bool isCompact: MD.MProp.size.isCompact

    readonly property var pageModel: [
        {
            name: 'components',
            source: 'qrc:/Qcm/Material/Example/Components.qml',
            icon: MD.Token.icon.widgets
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
        },
        {
            name: 'elevation',
            source: 'qrc:/Qcm/Material/Example/Elevation.qml',
            icon: MD.Token.icon.shadow
        },
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

    onPageIndexChanged: {
        m_container.replace(m_container.currentItem, pageModel[pageIndex].source, {});
    }

    Component.onCompleted: {
        pageIndexChanged();
        MD.Token.color.useSysColorSM = false;
    }

    MD.MProp.page: m_page_ctx

    MD.PageContext {
        id: m_page_ctx
        showHeader: true
        headerBackgroundOpacity: 0
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

                sourceComponent: MD.StandardDrawer {
                    model: root.pageModel
                    currentIndex: root.pageIndex

                    Behavior on implicitWidth {
                        NumberAnimation {
                            duration: MD.Token.duration.short4
                        }
                    }

                    onClicked: function (model) {
                        root.pageIndex = model.index;
                    }

                    drawerContent: ColumnLayout {
                        spacing: 0

                        MD.Space {
                            spacing: 16
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.minimumHeight: 0
                        }

                        ColumnLayout {
                            spacing: 8
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
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
                                        T.ActionGroup.group: m_palette_group
                                        icon.name: ''
                                        checkable: true
                                        checked: m_palette_view.currentIndex == index
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
                                        { name: 'Red', color: '#F44336' },
                                        { name: 'Pink', color: '#E91E63' },
                                        { name: 'Purple', color: '#9C27B0' },
                                        { name: 'Indigo', color: '#3F51B5' },
                                        { name: 'Teal', color: '#009688' },
                                        { name: 'LightGreen', color: '#8BC34A' },
                                        { name: 'Yellow', color: '#FFEB3B' },
                                        { name: 'Amber', color: '#FFC107' },
                                        { name: 'Orange', color: '#FF9800' }
                                    ]
                                    MD.ColorRadio {
                                        size: 28
                                        color: modelData.color
                                        checked: MD.Token.color.accentColor == color
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

            MD.StackView {
                id: m_container
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
                elevation: MD.Token.elevation.level2

                contentItem: RowLayout {
                    Repeater {
                        model: root.pageModel
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: 12 + children[0].implicitHeight + 16
                            required property var modelData
                            required property int index
                            MD.BarItem {
                                anchors.fill: parent
                                anchors.topMargin: 12
                                anchors.bottomMargin: 16
                                icon.name: parent.modelData.icon
                                text: parent.modelData.name
                                checked: root.pageIndex == parent.index
                                onClicked: {
                                    root.pageIndex = parent.index;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
