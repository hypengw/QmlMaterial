import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qcm.Material as MD

MD.Page {
    padding: 8

    MD.MatProp.page: m_page_ctx

    MD.PageContext {
        id: m_page_ctx
        showHeader: false
    }

    contentItem: ColumnLayout {
        id: content
        spacing: 0

        MD.TabBar {
            id: bar_test
            Layout.fillWidth: true
            spacing: 0
            clip: true
            currentIndex: 0

            onCurrentIndexChanged: {
                view_container.replace(view_container.currentItem, m_repeat.model[currentIndex].source, {});
            }

            Component.onCompleted: currentIndexChanged()

            Repeater {
                id: m_repeat
                model: [
                    {
                        name: 'components',
                        source: 'qrc:/Qcm/Material/Example/Components.qml'
                    },
                    {
                        name: 'color',
                        source: 'qrc:/Qcm/Material/Example/Color.qml'
                    },
                    {
                        name: 'shape',
                        source: 'qrc:/Qcm/Material/Example/Shape.qml'
                    },
                    {
                        name: 'elevation',
                        source: 'qrc:/Qcm/Material/Example/Elevation.qml'
                    },
                    {
                        name: 'typography',
                        source: 'qrc:/Qcm/Material/Example/Typography.qml'
                    }
                ]
                MD.TabButton {
                    action: Action {
                        text: modelData.name
                        onTriggered: {
                            bar_test.currentIndex = index;
                        }
                    }
                }
            }
        }

        MD.Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            padding: 0

            MD.StackView {
                id: view_container
                anchors.fill: parent
                initialItem: Item {}
            }
        }
    }
}
