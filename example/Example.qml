import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.Material as MD

MD.Page {
    id: root
    property int pageIndex: 0
    padding: 8
    title: 'Material 3'

    onPageIndexChanged: {
        m_container.replace(m_container.currentItem, m_repeat.model[pageIndex].source, {});
    }

    Component.onCompleted: pageIndexChanged()

    MD.MProp.page: m_page_ctx

    MD.PageContext {
        id: m_page_ctx
        showHeader: true
        headerBackgroundOpacity: 0
    }

    contentItem: RowLayout {
        id: content
        spacing: 0

        MD.StandardDrawer {
            Layout.fillHeight: true

            contentItem: ColumnLayout {
                MD.VerticalFlickable {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    topMargin: 0
                    bottomMargin: 12

                    ColumnLayout {
                        height: implicitHeight
                        width: parent.width
                        spacing: 0

                        Repeater {
                            id: m_repeat
                            model: [
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
                                },
                            ]
                            MD.DrawerItem {
                                Layout.fillWidth: true
                                action: MD.Action {
                                    icon.name: modelData.icon
                                    text: modelData.name
                                    checked: index == root.pageIndex
                                    onTriggered: {
                                        root.pageIndex = index;
                                    }
                                }
                            }
                        }
                    }
                }
                ColumnLayout {
                    Layout.maximumWidth: 200
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12
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

                    MD.Divider {}

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
                                    name: 'Oragen',
                                    color: '#FF9800'
                                },
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
}
